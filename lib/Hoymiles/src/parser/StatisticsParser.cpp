// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 - 2023 Thomas Basler and others
 */
#include "StatisticsParser.h"
#include "../Hoymiles.h"

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

const FieldId_t runtimeFields[] = {
    FLD_UDC,
    FLD_IDC,
    FLD_PDC,
    FLD_UAC,
    FLD_IAC,
    FLD_PAC,
    FLD_F,
    FLD_T,
    FLD_PF,
    FLD_Q,
    FLD_UAC_1N,
    FLD_UAC_2N,
    FLD_UAC_3N,
    FLD_UAC_12,
    FLD_UAC_23,
    FLD_UAC_31,
    FLD_IAC_1,
    FLD_IAC_2,
    FLD_IAC_3,
};

const FieldId_t dailyProductionFields[] = {
    FLD_YD,
};

StatisticsParser::StatisticsParser()
    : Parser()
{
    clearBuffer();
}

void StatisticsParser::setByteAssignment(const byteAssign_t* byteAssignment, uint8_t size)
{
    _byteAssignment = byteAssignment;
    _byteAssignmentSize = size;

    for (uint8_t i = 0; i < _byteAssignmentSize; i++) {
        if (_byteAssignment[i].div == CMD_CALC) {
            continue;
        }
        _expectedByteCount = max<uint8_t>(_expectedByteCount, _byteAssignment[i].start + _byteAssignment[i].num);
    }
}

uint8_t StatisticsParser::getExpectedByteCount()
{
    return _expectedByteCount;
}

void StatisticsParser::clearBuffer()
{
    memset(_payloadStatistic, 0, STATISTIC_PACKET_SIZE);
    _statisticLength = 0;
}

void StatisticsParser::appendFragment(uint8_t offset, uint8_t* payload, uint8_t len)
{
    if (offset + len > STATISTIC_PACKET_SIZE) {
        Hoymiles.getMessageOutput()->printf("FATAL: (%s, %d) stats packet too large for buffer\r\n", __FILE__, __LINE__);
        return;
    }
    memcpy(&_payloadStatistic[offset], payload, len);
    _statisticLength += len;
}

const byteAssign_t* StatisticsParser::getAssignmentByChannelField(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId)
{
    for (uint8_t i = 0; i < _byteAssignmentSize; i++) {
        if (_byteAssignment[i].type == type && _byteAssignment[i].ch == channel && _byteAssignment[i].fieldId == fieldId) {
            return &_byteAssignment[i];
        }
    }
    return NULL;
}

fieldSettings_t* StatisticsParser::getSettingByChannelField(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId)
{
    for (auto& i : _fieldSettings) {
        if (i.type == type && i.ch == channel && i.fieldId == fieldId) {
            return &i;
        }
    }
    return NULL;
}

float StatisticsParser::getChannelFieldValue(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId)
{
    const byteAssign_t* pos = getAssignmentByChannelField(type, channel, fieldId);
    fieldSettings_t* setting = getSettingByChannelField(type, channel, fieldId);

    if (pos == NULL) {
        return 0;
    }

    uint8_t ptr = pos->start;
    uint8_t end = ptr + pos->num;
    uint16_t div = pos->div;

    if (CMD_CALC != div) {
        // Value is a static value
        uint32_t val = 0;
        HOY_SEMAPHORE_TAKE();
        do {
            val <<= 8;
            val |= _payloadStatistic[ptr];
        } while (++ptr != end);
        HOY_SEMAPHORE_GIVE();

        float result;
        if (pos->isSigned && pos->num == 2) {
            result = static_cast<float>(static_cast<int16_t>(val));
        } else if (pos->isSigned && pos->num == 4) {
            result = static_cast<float>(static_cast<int32_t>(val));
        } else {
            result = static_cast<float>(val);
        }

        result /= static_cast<float>(div);
        if (setting != NULL && _statisticLength > 0) {
            result += setting->offset;
        }
        return result;
    } else {
        // Value has to be calculated
        return calcFunctions[pos->start].func(this, pos->num);
    }

    return 0;
}

bool StatisticsParser::setChannelFieldValue(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId, float value)
{
    const byteAssign_t* pos = getAssignmentByChannelField(type, channel, fieldId);
    fieldSettings_t* setting = getSettingByChannelField(type, channel, fieldId);

    if (pos == NULL) {
        return false;
    }

    uint8_t ptr = pos->start + pos->num - 1;
    uint8_t end = pos->start;
    uint16_t div = pos->div;

    if (CMD_CALC == div) {
        return false;
    }

    if (setting != NULL) {
        value -= setting->offset;
    }
    value *= static_cast<float>(div);

    uint32_t val = 0;
    if (pos->isSigned && pos->num == 2) {
        val = static_cast<uint32_t>(static_cast<int16_t>(value));
    } else if (pos->isSigned && pos->num == 4) {
        val = static_cast<uint32_t>(static_cast<int32_t>(value));
    } else {
        val = static_cast<uint32_t>(value);
    }

    HOY_SEMAPHORE_TAKE();
    do {
        _payloadStatistic[ptr] = val;
        val >>= 8;
    } while (--ptr >= end);
    HOY_SEMAPHORE_GIVE();

    return true;
}

String StatisticsParser::getChannelFieldValueString(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId)
{
    return String(
        getChannelFieldValue(type, channel, fieldId),
        static_cast<unsigned int>(getChannelFieldDigits(type, channel, fieldId)));
}

bool StatisticsParser::hasChannelFieldValue(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId)
{
    const byteAssign_t* pos = getAssignmentByChannelField(type, channel, fieldId);
    return pos != NULL;
}

const char* StatisticsParser::getChannelFieldUnit(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId)
{
    const byteAssign_t* pos = getAssignmentByChannelField(type, channel, fieldId);
    return units[pos->unitId];
}

const char* StatisticsParser::getChannelFieldName(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId)
{
    const byteAssign_t* pos = getAssignmentByChannelField(type, channel, fieldId);
    return fields[pos->fieldId];
}

uint8_t StatisticsParser::getChannelFieldDigits(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId)
{
    const byteAssign_t* pos = getAssignmentByChannelField(type, channel, fieldId);
    return pos->digits;
}

float StatisticsParser::getChannelFieldOffset(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId)
{
    fieldSettings_t* setting = getSettingByChannelField(type, channel, fieldId);
    if (setting != NULL) {
        return setting->offset;
    }
    return 0;
}

void StatisticsParser::setChannelFieldOffset(ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId, float offset)
{
    fieldSettings_t* setting = getSettingByChannelField(type, channel, fieldId);
    if (setting != NULL) {
        setting->offset = offset;
    } else {
        _fieldSettings.push_back({ type, channel, fieldId, offset });
    }
}

std::list<ChannelType_t> StatisticsParser::getChannelTypes()
{
    return {
        TYPE_AC,
        TYPE_DC,
        TYPE_INV
    };
}

const char* StatisticsParser::getChannelTypeName(ChannelType_t type)
{
    return channelsTypes[type];
}

std::list<ChannelNum_t> StatisticsParser::getChannelsByType(ChannelType_t type)
{
    std::list<ChannelNum_t> l;
    for (uint8_t i = 0; i < _byteAssignmentSize; i++) {
        if (_byteAssignment[i].type == type) {
            l.push_back(_byteAssignment[i].ch);
        }
    }
    l.unique();
    return l;
}

uint16_t StatisticsParser::getStringMaxPower(uint8_t channel)
{
    return _stringMaxPower[channel];
}

void StatisticsParser::setStringMaxPower(uint8_t channel, uint16_t power)
{
    if (channel < sizeof(_stringMaxPower) / sizeof(_stringMaxPower[0])) {
        _stringMaxPower[channel] = power;
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

void StatisticsParser::zeroRuntimeData()
{
    zeroFields(runtimeFields);
}

void StatisticsParser::zeroDailyData()
{
    zeroFields(dailyProductionFields);
}

void StatisticsParser::zeroFields(const FieldId_t* fields)
{
    // Loop all channels
    for (auto& t : getChannelTypes()) {
        for (auto& c : getChannelsByType(t)) {
            for (uint8_t i = 0; i < (sizeof(runtimeFields) / sizeof(runtimeFields[0])); i++) {
                if (hasChannelFieldValue(t, c, fields[i])) {
                    setChannelFieldValue(t, c, fields[i], 0);
                }
            }
        }
    }
}

static float calcYieldTotalCh0(StatisticsParser* iv, uint8_t arg0)
{
    float yield = 0;
    for (auto& channel : iv->getChannelsByType(TYPE_DC)) {
        yield += iv->getChannelFieldValue(TYPE_DC, channel, FLD_YT);
    }
    return yield;
}

static float calcYieldDayCh0(StatisticsParser* iv, uint8_t arg0)
{
    float yield = 0;
    for (auto& channel : iv->getChannelsByType(TYPE_DC)) {
        yield += iv->getChannelFieldValue(TYPE_DC, channel, FLD_YD);
    }
    return yield;
}

// arg0 = channel of source
static float calcUdcCh(StatisticsParser* iv, uint8_t arg0)
{
    return iv->getChannelFieldValue(TYPE_DC, static_cast<ChannelNum_t>(arg0), FLD_UDC);
}

static float calcPowerDcCh0(StatisticsParser* iv, uint8_t arg0)
{
    float dcPower = 0;
    for (auto& channel : iv->getChannelsByType(TYPE_DC)) {
        dcPower += iv->getChannelFieldValue(TYPE_DC, channel, FLD_PDC);
    }
    return dcPower;
}

// arg0 = channel
static float calcEffiencyCh0(StatisticsParser* iv, uint8_t arg0)
{
    float acPower = 0;
    for (auto& channel : iv->getChannelsByType(TYPE_AC)) {
        acPower += iv->getChannelFieldValue(TYPE_AC, channel, FLD_PAC);
    }

    float dcPower = 0;
    for (auto& channel : iv->getChannelsByType(TYPE_DC)) {
        dcPower += iv->getChannelFieldValue(TYPE_DC, channel, FLD_PDC);
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
        if (iv->getStringMaxPower(arg0) > 0)
            return iv->getChannelFieldValue(TYPE_DC, static_cast<ChannelNum_t>(arg0), FLD_PDC) / iv->getStringMaxPower(arg0) * 100.0f;
    }
    return 0.0;
}
