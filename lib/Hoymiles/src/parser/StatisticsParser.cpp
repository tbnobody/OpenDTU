// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "StatisticsParser.h"

static float calcYieldTotalCh0(StatisticsParser* iv, uint8_t arg0);
static float calcYieldDayCh0(StatisticsParser* iv, uint8_t arg0);
static float calcUdcCh(StatisticsParser* iv, uint8_t arg0);
static float calcPowerDcCh0(StatisticsParser* iv, uint8_t arg0);
static float calcEffiencyCh0(StatisticsParser* iv, uint8_t arg0);
static float calcIrradiation(StatisticsParser* iv, uint8_t arg0);

using func_t = float(StatisticsParser*, uint8_t);

struct calcFunc_t {
    uint8_t funcId; // unique id
    func_t* func; // function pointer
};

const calcFunc_t calcFunctions[] = {
    { CALC_YT_CH0, &calcYieldTotalCh0 },
    { CALC_YD_CH0, &calcYieldDayCh0 },
    { CALC_UDC_CH, &calcUdcCh },
    { CALC_PDC_CH0, &calcPowerDcCh0 },
    { CALC_EFF_CH0, &calcEffiencyCh0 },
    { CALC_IRR_CH, &calcIrradiation }
};

void StatisticsParser::setByteAssignment(const byteAssign_t* byteAssignment, const uint8_t count)
{
    _byteAssignment = byteAssignment;
    _byteAssignmentCount = count;
}

void StatisticsParser::clearBuffer()
{
    memset(_payloadStatistic, 0, STATISTIC_PACKET_SIZE);
    _statisticLength = 0;
}

void StatisticsParser::appendFragment(uint8_t offset, uint8_t* payload, uint8_t len)
{
    if (offset + len > STATISTIC_PACKET_SIZE) {
        Serial.printf("FATAL: (%s, %d) stats packet too large for buffer\n", __FILE__, __LINE__);
        return;
    }
    memcpy(&_payloadStatistic[offset], payload, len);
    _statisticLength += len;
}

uint8_t StatisticsParser::getAssignIdxByChannelField(uint8_t channel, uint8_t fieldId)
{
    const byteAssign_t* b = _byteAssignment;

    uint8_t pos;
    for (pos = 0; pos < _byteAssignmentCount; pos++) {
        if (b[pos].ch == channel && b[pos].fieldId == fieldId) {
            return pos;
        }
    }
    return 0xff;
}

float StatisticsParser::getChannelFieldValue(uint8_t channel, uint8_t fieldId)
{
    uint8_t pos = getAssignIdxByChannelField(channel, fieldId);
    if (pos == 0xff) {
        return 0;
    }

    const byteAssign_t* b = _byteAssignment;

    uint8_t ptr = b[pos].start;
    uint8_t end = ptr + b[pos].num;
    uint16_t div = b[pos].div;

    if (CMD_CALC != div) {
        // Value is a static value
        uint32_t val = 0;
        do {
            val <<= 8;
            val |= _payloadStatistic[ptr];
        } while (++ptr != end);

        float result;
        if (b[pos].isSigned && b[pos].num == 2) {
            result = static_cast<float>(static_cast<int16_t>(val));
        } else if (b[pos].isSigned && b[pos].num == 4) {
            result = static_cast<float>(static_cast<int32_t>(val));
        } else {
            result = static_cast<float>(val);
        }

        result /= static_cast<float>(div);
        return result;
    } else {
        // Value has to be calculated
        return calcFunctions[b[pos].start].func(this, b[pos].num);
    }

    return 0;
}

bool StatisticsParser::hasChannelFieldValue(uint8_t channel, uint8_t fieldId)
{
    uint8_t pos = getAssignIdxByChannelField(channel, fieldId);
    return pos != 0xff;
}

const char* StatisticsParser::getChannelFieldUnit(uint8_t channel, uint8_t fieldId)
{
    uint8_t pos = getAssignIdxByChannelField(channel, fieldId);
    const byteAssign_t* b = _byteAssignment;

    return units[b[pos].unitId];
}

const char* StatisticsParser::getChannelFieldName(uint8_t channel, uint8_t fieldId)
{
    uint8_t pos = getAssignIdxByChannelField(channel, fieldId);
    const byteAssign_t* b = _byteAssignment;

    return fields[b[pos].fieldId];
}

uint8_t StatisticsParser::getChannelFieldDigits(uint8_t channel, uint8_t fieldId)
{
    uint8_t pos = getAssignIdxByChannelField(channel, fieldId);
    const byteAssign_t* b = _byteAssignment;

    switch (b[pos].div) {
    case 1:
        return 0;
    case 10:
        return 1;
    case 100:
        return 2;
    case 1000:
        return 3;
    default:
        return 2;
    }
}

uint8_t StatisticsParser::getChannelCount()
{
    const byteAssign_t* b = _byteAssignment;
    uint8_t cnt = 0;
    for (uint8_t pos = 0; pos < _byteAssignmentCount; pos++) {
        if (b[pos].ch > cnt) {
            cnt = b[pos].ch;
        }
    }

    return cnt;
}

uint16_t StatisticsParser::getChannelMaxPower(uint8_t channel)
{
    return _chanMaxPower[channel];
}

void StatisticsParser::setChannelMaxPower(uint8_t channel, uint16_t power)
{
    if (channel < CH4) {
        _chanMaxPower[channel] = power;
    }
}

void StatisticsParser::resetRxFailureCount()
{
    _rxFailureCount = 0;
}

void StatisticsParser::incrementRxFailureCount()
{
    _rxFailureCount++;
}

uint32_t StatisticsParser::getRxFailureCount()
{
    return _rxFailureCount;
}

static float calcYieldTotalCh0(StatisticsParser* iv, uint8_t arg0)
{
    float yield = 0;
    for (uint8_t i = 1; i <= iv->getChannelCount(); i++) {
        yield += iv->getChannelFieldValue(i, FLD_YT);
    }
    return yield;
}

static float calcYieldDayCh0(StatisticsParser* iv, uint8_t arg0)
{
    float yield = 0;
    for (uint8_t i = 1; i <= iv->getChannelCount(); i++) {
        yield += iv->getChannelFieldValue(i, FLD_YD);
    }
    return yield;
}

// arg0 = channel of source
static float calcUdcCh(StatisticsParser* iv, uint8_t arg0)
{
    return iv->getChannelFieldValue(arg0, FLD_UDC);
}

static float calcPowerDcCh0(StatisticsParser* iv, uint8_t arg0)
{
    float dcPower = 0;
    for (uint8_t i = 1; i <= iv->getChannelCount(); i++) {
        dcPower += iv->getChannelFieldValue(i, FLD_PDC);
    }
    return dcPower;
}

// arg0 = channel
static float calcEffiencyCh0(StatisticsParser* iv, uint8_t arg0)
{
    float acPower = iv->getChannelFieldValue(CH0, FLD_PAC);
    float dcPower = 0;
    for (uint8_t i = 1; i <= iv->getChannelCount(); i++) {
        dcPower += iv->getChannelFieldValue(i, FLD_PDC);
    }
    if (dcPower > 0) {
        return acPower / dcPower * 100.0f;
    }

    return 0.0;
}

// arg0 = channel
static float calcIrradiation(StatisticsParser* iv, uint8_t arg0)
{
    if (NULL != iv) {
        if (iv->getChannelMaxPower(arg0 - 1) > 0)
            return iv->getChannelFieldValue(arg0, FLD_PDC) / iv->getChannelMaxPower(arg0 - 1) * 100.0f;
    }
    return 0.0;
}
