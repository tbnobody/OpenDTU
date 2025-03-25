// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "FrequencyManagerAbstract.h"

class FrequencyManager_CMT:public FrequencyManagerAbstract {
public:
    explicit FrequencyManager_CMT(InverterAbstract* inv);

    uint32_t getTXFrequency(CommandAbstract& cmd);
    uint32_t getRXFrequency(CommandAbstract& cmd);

    void processRXResult(CommandAbstract *cmd, uint8_t verify_fragments_result);
    // if ok -> set lastworkingfrequency
    // if not -> noop

    bool shouldSendChangeChannelCommand();
    void startNextFetch();

private:
    uint32_t _lastWorkingFrequency = 0;
    uint32_t _getFrequency(CommandAbstract& cmd);
    int _failedFetchCount = -1;
};

uint32_t _get_cmt_search_frequency(int failed_fetch_count, int cmd_send_count, uint32_t inverter_target_frequency, uint32_t min_frequency, uint32_t max_frequency, uint32_t legal_min_frequency, uint32_t legal_max_frequency, uint32_t channel_width);
