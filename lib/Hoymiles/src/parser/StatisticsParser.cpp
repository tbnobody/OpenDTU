#include "StatisticsParser.h"

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

        return (float)(val) / (float)(div);
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