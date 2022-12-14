// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "InverterAbstract.h"
#include "crc.h"
#include <cstring>

InverterAbstract::InverterAbstract(uint64_t serial)
{
    _serial.u64 = serial;

    char serial_buff[sizeof(uint64_t) * 8 + 1];
    snprintf(serial_buff, sizeof(serial_buff), "%0x%08x",
        ((uint32_t)((serial >> 32) & 0xFFFFFFFF)),
        ((uint32_t)(serial & 0xFFFFFFFF)));
    _serialString = serial_buff;

    _alarmLogParser.reset(new AlarmLogParser());
    _devInfoParser.reset(new DevInfoParser());
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
    _statisticsParser.get()->setByteAssignment(getByteAssignment(), getAssignmentCount());
}

uint64_t InverterAbstract::serial()
{
    return _serial.u64;
}

const String& InverterAbstract::serialString()
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

const char* InverterAbstract::name()
{
    return _name;
}

bool InverterAbstract::isProducing()
{
    if (!Statistics()->hasChannelFieldValue(CH0, FLD_PAC)) {
        return false;
    }

    return Statistics()->getChannelFieldValue(CH0, FLD_PAC) > 0;
}

bool InverterAbstract::isReachable()
{
    return Statistics()->getRxFailureCount() <= MAX_ONLINE_FAILURE_COUNT;
}

AlarmLogParser* InverterAbstract::EventLog()
{
    return _alarmLogParser.get();
}

DevInfoParser* InverterAbstract::DevInfo()
{
    return _devInfoParser.get();
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

void InverterAbstract::addRxFragment(uint8_t fragment[], uint8_t len)
{
    if (len < 11) {
        Serial.printf("FATAL: (%s, %d) fragment too short\n", __FILE__, __LINE__);
        return;
    }

    if (len - 11 > MAX_RF_PAYLOAD_SIZE) {
        Serial.printf("FATAL: (%s, %d) fragment too large\n", __FILE__, __LINE__);
        return;
    }

    uint8_t fragmentCount = fragment[9];
    if (fragmentCount == 0) {
        Serial.println("ERROR: fragment number zero received and ignored");
        return;
    }

    if ((fragmentCount & 0b01111111) < MAX_RF_FRAGMENT_COUNT) {
        // Packets with 0x81 will be seen as 1
        memcpy(_rxFragmentBuffer[(fragmentCount & 0b01111111) - 1].fragment, &fragment[10], len - 11);
        _rxFragmentBuffer[(fragmentCount & 0b01111111) - 1].len = len - 11;
        _rxFragmentBuffer[(fragmentCount & 0b01111111) - 1].mainCmd = fragment[0];
        _rxFragmentBuffer[(fragmentCount & 0b01111111) - 1].wasReceived = true;

        if ((fragmentCount & 0b01111111) > _rxFragmentLastPacketId) {
            _rxFragmentLastPacketId = fragmentCount & 0b01111111;
        }
    }

    // 0b10000000 == 0x80
    if ((fragmentCount & 0b10000000) == 0b10000000) {
        _rxFragmentMaxPacketId = fragmentCount & 0b01111111;
    }
}

// Returns Zero on Success or the Fragment ID for retransmit or error code
uint8_t InverterAbstract::verifyAllFragments(CommandAbstract* cmd)
{
    // All missing
    if (_rxFragmentLastPacketId == 0) {
        Serial.println(F("All missing"));
        if (cmd->getSendCount() <= MAX_RESEND_COUNT) {
            return FRAGMENT_ALL_MISSING_RESEND;
        } else {
            cmd->gotTimeout(this);
            return FRAGMENT_ALL_MISSING_TIMEOUT;
        }
    }

    // Last fragment is missing (thte one with 0x80)
    if (_rxFragmentMaxPacketId == 0) {
        Serial.println(F("Last missing"));
        if (_rxFragmentRetransmitCnt++ < MAX_RETRANSMIT_COUNT) {
            return _rxFragmentLastPacketId + 1;
        } else {
            cmd->gotTimeout(this);
            return FRAGMENT_RETRANSMIT_TIMEOUT;
        }
    }

    // Middle fragment is missing
    for (uint8_t i = 0; i < _rxFragmentMaxPacketId - 1; i++) {
        if (!_rxFragmentBuffer[i].wasReceived) {
            Serial.println(F("Middle missing"));
            if (_rxFragmentRetransmitCnt++ < MAX_RETRANSMIT_COUNT) {
                return i + 1;
            } else {
                cmd->gotTimeout(this);
                return FRAGMENT_RETRANSMIT_TIMEOUT;
            }
        }
    }

    if (!cmd->handleResponse(this, _rxFragmentBuffer, _rxFragmentMaxPacketId)) {
        cmd->gotTimeout(this);
        return FRAGMENT_HANDLE_ERROR;
    }

    return FRAGMENT_OK;
}