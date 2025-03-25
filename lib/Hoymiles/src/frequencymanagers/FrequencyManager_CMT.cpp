// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2024 Thomas Basler and others
 */

#include "FrequencyManager_CMT.h"
#include "Hoymiles.h"
#include "inverters/InverterAbstract.h"
#include "commands/CommandAbstract.h"

FrequencyManager_CMT::FrequencyManager_CMT(InverterAbstract* inv):FrequencyManagerAbstract(inv) {
}

bool isChangeChannelCmd(CommandAbstract &cmd) {
    return cmd.getDataPayload()[0] == 0x56; // @todo(tbnobody) Bad hack to identify ChannelChange Command
}

uint32_t FrequencyManager_CMT::getTXFrequency(CommandAbstract& cmd) {
    uint32_t freq = this->_getFrequency(cmd);
    if (isChangeChannelCmd(cmd)) {
        return Hoymiles.getRadioCmt()->getInvBootFrequency();
    }
    return freq;
}

uint32_t FrequencyManager_CMT::getRXFrequency(CommandAbstract& cmd) {
    if (isChangeChannelCmd(cmd)) {
        return Hoymiles.getRadioCmt()->getInverterTargetFrequency();
    }
    return this->_getFrequency(cmd);
}

// https://stackoverflow.com/a/14997413
inline int positive_modulo(int i, int n) {
    return (i % n + n) % n;
}

uint32_t _get_cmt_search_frequency(int failed_fetch_count, int cmd_send_count, uint32_t inverter_target_frequency, uint32_t min_frequency, uint32_t max_frequency, uint32_t legal_min_frequency, uint32_t legal_max_frequency, uint32_t channel_width) {
    // cmt_send_count gets incremented in sendEsbPacket just before we are called; we do -1 to undo this
    // _getFrequency also grabs the first transmission, setting it to inverterTargetFrequency. we do -1 to undo this as well
    int cmd_transmissions_we_did = cmd_send_count-2; // how often this particular command has already been sent on a freq determined by this function

    // failed_fetch_count starts at 1, due to how startsNextFetch counts
    int fetches_we_did = failed_fetch_count - 1;

    int offset2 = (fetches_we_did + (cmd_transmissions_we_did/2)) % 20;
    int offset3 = offset2 * (cmd_transmissions_we_did%2==0?-1:1);

    uint32_t min_usable_freq = max(min_frequency, legal_min_frequency);
    uint32_t max_usable_freq = min(max_frequency, legal_max_frequency);
    int min_offset = -((inverter_target_frequency - min_usable_freq)/channel_width);
    int max_offset = (max_usable_freq - inverter_target_frequency)/channel_width;

    int final_offset = (positive_modulo(offset3 - min_offset, max_offset + 1 - min_offset)) + min_offset;

    int ret = inverter_target_frequency + (final_offset*channel_width);
    // Hoymiles.getMessageOutput()->printf("cmt_search_frequency min_offset=%" PRId32 " max_offset=%" PRId32 "\r\n", min_offset, max_offset);
    // Hoymiles.getMessageOutput()->printf("cmt_search_frequency failed_fetch_count=%" PRId32 " cmd_send_count=%" PRId32 " now trying %.3f MHz\r\n", failed_fetch_count, cmd_send_count, ret / 1000000.0);
    // Hoymiles.getMessageOutput()->printf("cmt_search_frequency offset2 %" PRId32 " offset3 %" PRId32 " final_offset %" PRId32 " \r\n", offset2, offset3, final_offset);
    return ret;
}

uint32_t FrequencyManager_CMT::_getFrequency(CommandAbstract& cmd) {
    HoymilesRadio_CMT *radio = Hoymiles.getRadioCmt();
    uint32_t tgt_freq = radio->getInverterTargetFrequency();
    int cmd_retransmit_count = cmd.getSendCount()-1;

    if(this->_inv->isReachable() || this->_failedFetchCount <= 0) {
        // _lastWorkingFrequency was working or is 0.
        bool isOnTgtFreq = this->_lastWorkingFrequency == tgt_freq || _lastWorkingFrequency == 0;
        if(isOnTgtFreq) {
            return tgt_freq;
        } else {
            // we are still sending ChangeChannelCommands, so we should keep checking if it worked
            if(cmd_retransmit_count%2==0) {
                return this->_lastWorkingFrequency;
            } else {
                return tgt_freq;
            }
        }
    } else {
        // we are sending ChangeChannelCommands, so we should keep checking if it worked
        if(cmd_retransmit_count == 0) {
            return tgt_freq;
        }
        // start searching
        return _get_cmt_search_frequency(this->_failedFetchCount, cmd.getSendCount(), tgt_freq, radio->getMinFrequency(), radio->getMaxFrequency(), radio->getLegalMinFrequency(), radio->getLegalMaxFrequency(), radio->getChannelWidth());
    }
}

void FrequencyManager_CMT::processRXResult(CommandAbstract *cmd, uint8_t verify_fragments_result) {
    if(verify_fragments_result == FRAGMENT_OK) {
        this->_lastWorkingFrequency = this->getRXFrequency(*cmd);
        this->_failedFetchCount = -1;
        this->_inv->RadioStats.RxLastFrequency = this->_lastWorkingFrequency;
    }

    // FRAGMENT_ALL_MISSING_RESEND
    // FRAGMENT_ALL_MISSING_TIMEOUT
    // FRAGMENT_HANDLE_ERROR
}


bool FrequencyManager_CMT::shouldSendChangeChannelCommand() {
    if (!this->_inv->isReachable()) {
        return true;
    } else {
        if (this->_lastWorkingFrequency == 0) {
            return false; // 0 means FRAGMENT_OK was never received, i.e. no packet since OpenDTU boot. return false to preserve old startup sequence
        }
        return this->_lastWorkingFrequency != Hoymiles.getRadioCmt()->getInverterTargetFrequency();
    }
}

void FrequencyManager_CMT::startNextFetch() {
    if(this->_inv->isReachable()) {
        this->_failedFetchCount = 0;
    } else {
        this->_failedFetchCount++;
    }
}
