// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "InverterAbstract.h"
#include "../Hoymiles.h"
#include "crc.h"
#include <cstring>

InverterAbstract::InverterAbstract(HoymilesRadio* radio, const uint64_t serial)
{
    _serial.u64 = serial;
    _radio = radio;

    char serial_buff[sizeof(uint64_t) * 8 + 1];
    snprintf(serial_buff, sizeof(serial_buff), "%0" PRIx32 "%08" PRIx32,
        static_cast<uint32_t>((serial >> 32) & 0xFFFFFFFF),
        static_cast<uint32_t>(serial & 0xFFFFFFFF));
    _serialString = serial_buff;

    _alarmLogParser.reset(new AlarmLogParser());
    _devInfoParser.reset(new DevInfoParser());
    _gridProfileParser.reset(new GridProfileParser());
    _powerCommandParser.reset(new PowerCommandParser());
    _statisticsParser.reset(new StatisticsParser());
    _systemConfigParaParser.reset(new SystemConfigParaParser());
}

void InverterAbstract::init()
{
    // This has to be done here because:
    // Not possible in constructor --> virtual function
    // Not possible in verifyAllFragments --> Because no data if nothing is ever received
    // It has to be executed because otherwise the getChannelCount method in stats always returns 0
    _statisticsParser.get()->setByteAssignment(getByteAssignment(), getByteAssignmentSize());
}

uint64_t InverterAbstract::serial() const
{
    return _serial.u64;
}

const String& InverterAbstract::serialString() const
{
    return _serialString;
}

void InverterAbstract::setName(const char* name)
{
    uint8_t len = strlen(name);
    if (len + 1 > MAX_NAME_LENGTH) {
        len = MAX_NAME_LENGTH - 1;
    }
    strncpy(_name, name, len);
    _name[len] = '\0';
}

const char* InverterAbstract::name() const
{
    return _name;
}

bool InverterAbstract::isProducing()
{
    float totalAc = 0;
    for (auto& c : Statistics()->getChannelsByType(TYPE_AC)) {
        if (Statistics()->hasChannelFieldValue(TYPE_AC, c, FLD_PAC)) {
            totalAc += Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_PAC);
        }
    }

    return _enablePolling && totalAc > 0;
}

bool InverterAbstract::isReachable()
{
    return _enablePolling && Statistics()->getRxFailureCount() <= _reachableThreshold;
}

void InverterAbstract::setEnablePolling(const bool enabled)
{
    _enablePolling = enabled;
}

bool InverterAbstract::getEnablePolling() const
{
    return _enablePolling;
}

void InverterAbstract::setEnableCommands(const bool enabled)
{
    _enableCommands = enabled;
}

bool InverterAbstract::getEnableCommands() const
{
    return _enableCommands;
}

void InverterAbstract::setReachableThreshold(const uint8_t threshold)
{
    _reachableThreshold = threshold;
}

uint8_t InverterAbstract::getReachableThreshold() const
{
    return _reachableThreshold;
}

void InverterAbstract::setZeroValuesIfUnreachable(const bool enabled)
{
    _zeroValuesIfUnreachable = enabled;
}

bool InverterAbstract::getZeroValuesIfUnreachable() const
{
    return _zeroValuesIfUnreachable;
}

void InverterAbstract::setZeroYieldDayOnMidnight(const bool enabled)
{
    _zeroYieldDayOnMidnight = enabled;
}

bool InverterAbstract::getZeroYieldDayOnMidnight() const
{
    return _zeroYieldDayOnMidnight;
}

void InverterAbstract::setClearEventlogOnMidnight(const bool enabled)
{
    _clearEventlogOnMidnight = enabled;
}

bool InverterAbstract::getClearEventlogOnMidnight() const
{
    return _clearEventlogOnMidnight;
}

int8_t InverterAbstract::getLastRssi() const
{
    return _lastRssi;
}

bool InverterAbstract::sendChangeChannelRequest()
{
    return false;
}

HoymilesRadio* InverterAbstract::getRadio()
{
    return _radio;
}

AlarmLogParser* InverterAbstract::EventLog()
{
    return _alarmLogParser.get();
}

DevInfoParser* InverterAbstract::DevInfo()
{
    return _devInfoParser.get();
}

GridProfileParser* InverterAbstract::GridProfile()
{
    return _gridProfileParser.get();
}

PowerCommandParser* InverterAbstract::PowerCommand()
{
    return _powerCommandParser.get();
}

StatisticsParser* InverterAbstract::Statistics()
{
    return _statisticsParser.get();
}

SystemConfigParaParser* InverterAbstract::SystemConfigPara()
{
    return _systemConfigParaParser.get();
}

void InverterAbstract::clearRxFragmentBuffer()
{
    memset(_rxFragmentBuffer, 0, MAX_RF_FRAGMENT_COUNT * sizeof(fragment_t));
    _rxFragmentMaxPacketId = 0;
    _rxFragmentLastPacketId = 0;
    _rxFragmentRetransmitCnt = 0;
}

void InverterAbstract::addRxFragment(const uint8_t fragment[], const uint8_t len, const int8_t rssi)
{
    _lastRssi = rssi;

    if (len < 11) {
        Hoymiles.getMessageOutput()->printf("FATAL: (%s, %d) fragment too short\r\n", __FILE__, __LINE__);
        return;
    }

    if (len - 11 > MAX_RF_PAYLOAD_SIZE) {
        Hoymiles.getMessageOutput()->printf("FATAL: (%s, %d) fragment too large\r\n", __FILE__, __LINE__);
        return;
    }

    const uint8_t fragmentCount = fragment[9];

    // Packets with 0x81 will be seen as 1
    const uint8_t fragmentId = fragmentCount & 0b01111111; // fragmentId is 1 based

    if (fragmentId == 0) {
        Hoymiles.getMessageOutput()->println("ERROR: fragment id zero received and ignored");
        return;
    }

    if (fragmentId >= MAX_RF_FRAGMENT_COUNT) {
        Hoymiles.getMessageOutput()->printf("ERROR: fragment id %" PRIu8 " is too large for buffer and ignored\r\n", fragmentId);
        return;
    }

    memcpy(_rxFragmentBuffer[fragmentId - 1].fragment, &fragment[10], len - 11);
    _rxFragmentBuffer[fragmentId - 1].len = len - 11;
    _rxFragmentBuffer[fragmentId - 1].mainCmd = fragment[0];
    _rxFragmentBuffer[fragmentId - 1].wasReceived = true;

    if (fragmentId > _rxFragmentLastPacketId) {
        _rxFragmentLastPacketId = fragmentId;
    }

    // 0b10000000 == 0x80
    if ((fragmentCount & 0b10000000) == 0b10000000) {
        _rxFragmentMaxPacketId = fragmentId;
    }
}

// Returns Zero on Success or the Fragment ID for retransmit or error code
uint8_t InverterAbstract::verifyAllFragments(CommandAbstract& cmd)
{
    // All missing
    if (_rxFragmentLastPacketId == 0) {
        Hoymiles.getMessageOutput()->println("All missing");
        if (cmd.getSendCount() <= cmd.getMaxResendCount()) {
            return FRAGMENT_ALL_MISSING_RESEND;
        } else {
            cmd.gotTimeout();
            return FRAGMENT_ALL_MISSING_TIMEOUT;
        }
    }

    // Last fragment is missing (the one with 0x80)
    if (_rxFragmentMaxPacketId == 0) {
        Hoymiles.getMessageOutput()->println("Last missing");
        if (_rxFragmentRetransmitCnt++ < cmd.getMaxRetransmitCount()) {
            return _rxFragmentLastPacketId + 1;
        } else {
            cmd.gotTimeout();
            return FRAGMENT_RETRANSMIT_TIMEOUT;
        }
    }

    // Middle fragment is missing
    for (uint8_t i = 0; i < _rxFragmentMaxPacketId - 1; i++) {
        if (!_rxFragmentBuffer[i].wasReceived) {
            Hoymiles.getMessageOutput()->println("Middle missing");
            if (_rxFragmentRetransmitCnt++ < cmd.getMaxRetransmitCount()) {
                return i + 1;
            } else {
                cmd.gotTimeout();
                return FRAGMENT_RETRANSMIT_TIMEOUT;
            }
        }
    }

    if (!cmd.handleResponse(_rxFragmentBuffer, _rxFragmentMaxPacketId)) {
        cmd.gotTimeout();
        return FRAGMENT_HANDLE_ERROR;
    }

    return FRAGMENT_OK;
}

void InverterAbstract::performDailyTask()
{
    // Have to reset the offets first, otherwise it will
    // Substract the offset from zero which leads to a high value
    Statistics()->resetYieldDayCorrection();
    if (getZeroYieldDayOnMidnight()) {
        Statistics()->zeroDailyData();
    }
    if (getClearEventlogOnMidnight()) {
        EventLog()->clearBuffer();
    }
    resetRadioStats();
}

void InverterAbstract::resetRadioStats()
{
    RadioStats = {};
}
