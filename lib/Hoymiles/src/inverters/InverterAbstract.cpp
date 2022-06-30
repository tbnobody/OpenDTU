#include "InverterAbstract.h"
#include "crc.h"
#include <cstring>

InverterAbstract::InverterAbstract(uint64_t serial)
{
    _serial.u64 = serial;
    memset(_payloadStats, 0, MAX_RF_FRAGMENT_COUNT * MAX_RF_PAYLOAD_SIZE);
}

uint64_t InverterAbstract::serial()
{
    return _serial.u64;
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

void InverterAbstract::clearRxFragmentBuffer()
{
    memset(_rxFragmentBuffer, 0, MAX_RF_FRAGMENT_COUNT * MAX_RF_PAYLOAD_SIZE);
    _rxFragmentMaxPacketId = 0;
    _rxFragmentLastPacketId = 0;
    _rxFragmentRetransmitCnt = 0;
}

void InverterAbstract::addRxFragment(uint8_t fragment[], uint8_t len)
{
    uint8_t fragmentCount = fragment[9];
    if ((fragmentCount & 0b01111111) < MAX_RF_FRAGMENT_COUNT) {
        // Packets with 0x81 will be seen as 1
        memcpy(_rxFragmentBuffer[(fragmentCount & 0b01111111) - 1].fragment, &fragment[10], len - 11);
        _rxFragmentBuffer[(fragmentCount & 0b01111111) - 1].len = len - 11;

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
uint8_t InverterAbstract::verifyAllFragments()
{
    // All missing
    if (_rxFragmentLastPacketId == 0) {
        Serial.println(F("All missing"));
        return FRAGMENT_ALL_MISSING;
    }

    // Last fragment is missing (thte one with 0x80)
    if (_rxFragmentMaxPacketId == 0) {
        Serial.println(F("Last missing"));
        if (_rxFragmentRetransmitCnt++ < MAX_RETRANSMIT_COUNT) {
            return _rxFragmentLastPacketId + 1;
        } else {
            return FRAGMENT_RETRANSMIT_TIMEOUT;
        }
    }

    // Middle fragment is missing
    for (uint8_t i = 0; i < _rxFragmentMaxPacketId - 1; i++) {
        if (_rxFragmentBuffer[i].len == 0) {
            Serial.println(F("Middle missing"));
            if (_rxFragmentRetransmitCnt++ < MAX_RETRANSMIT_COUNT) {
                return i + 1;
            } else {
                return FRAGMENT_RETRANSMIT_TIMEOUT;
            }
        }
    }

    // All fragments are available --> Check CRC
    uint16_t crc = 0xffff, crcRcv;

    for (uint8_t i = 0; i < _rxFragmentMaxPacketId; i++) {
        if (i == _rxFragmentMaxPacketId - 1) {
            // Last packet
            crc = crc16(_rxFragmentBuffer[i].fragment, _rxFragmentBuffer[i].len - 2, crc);
            crcRcv = (_rxFragmentBuffer[i].fragment[_rxFragmentBuffer[i].len - 2] << 8)
                | (_rxFragmentBuffer[i].fragment[_rxFragmentBuffer[i].len - 1]);
        } else {
            crc = crc16(_rxFragmentBuffer[i].fragment, _rxFragmentBuffer[i].len, crc);
        }
    }

    if (crc != crcRcv) {
        return FRAGMENT_CRC_ERROR;
    }

    if (getLastRequest() == RequestType::Stats) {
        // Move all fragments into target buffer
        memset(_payloadStats, 0, MAX_RF_FRAGMENT_COUNT * MAX_RF_PAYLOAD_SIZE);
        uint8_t offs = 0;
        for (uint8_t i = 0; i < _rxFragmentMaxPacketId; i++) {
            memcpy(&_payloadStats[offs], _rxFragmentBuffer[i].fragment, _rxFragmentBuffer[i].len);
            offs += (_rxFragmentBuffer[i].len);
        }
        _lastStatsUpdate = millis();
    }

    return FRAGMENT_OK;
}

uint32_t InverterAbstract::getLastStatsUpdate()
{
    return _lastStatsUpdate;
}

void InverterAbstract::setLastRequest(RequestType request)
{
    _lastRequest = request;
}

RequestType InverterAbstract::getLastRequest()
{
    return _lastRequest;
}

uint8_t InverterAbstract::getChannelCount()
{
    const byteAssign_t* b = getByteAssignment();
    uint8_t cnt = 0;
    for (uint8_t pos = 0; pos < getAssignmentCount(); pos++) {
        if (b[pos].ch > cnt) {
            cnt = b[pos].ch;
        }
    }

    return cnt;
}

uint16_t InverterAbstract::getChannelMaxPower(uint8_t channel)
{
    return _chanMaxPower[channel];
}

void InverterAbstract::setChannelMaxPower(uint8_t channel, uint16_t power)
{
    if (channel < CH4) {
        _chanMaxPower[channel] = power;
    }
}

uint8_t InverterAbstract::getAssignIdxByChannelField(uint8_t channel, uint8_t fieldId)
{
    const byteAssign_t* b = getByteAssignment();

    uint8_t pos;
    for (pos = 0; pos < getAssignmentCount(); pos++) {
        if (b[pos].ch == channel && b[pos].fieldId == fieldId) {
            return pos;
        }
    }
    return 0xff;
}

float InverterAbstract::getChannelFieldValue(uint8_t channel, uint8_t fieldId)
{
    uint8_t pos = getAssignIdxByChannelField(channel, fieldId);
    if (pos == 0xff) {
        return 0;
    }

    const byteAssign_t* b = getByteAssignment();

    uint8_t ptr = b[pos].start;
    uint8_t end = ptr + b[pos].num;
    uint16_t div = b[pos].div;

    if (CMD_CALC != div) {
        // Value is a static value
        uint32_t val = 0;
        do {
            val <<= 8;
            val |= _payloadStats[ptr];
        } while (++ptr != end);

        return (float)(val) / (float)(div);
    } else {
        // Value has to be calculated
        return calcFunctions[b[pos].start].func(this, b[pos].num);
    }

    return 0;
}

bool InverterAbstract::hasChannelFieldValue(uint8_t channel, uint8_t fieldId)
{
    uint8_t pos = getAssignIdxByChannelField(channel, fieldId);
    return pos != 0xff;
}

const char* InverterAbstract::getChannelFieldUnit(uint8_t channel, uint8_t fieldId)
{
    uint8_t pos = getAssignIdxByChannelField(channel, fieldId);
    const byteAssign_t* b = getByteAssignment();

    return units[b[pos].unitId];
}

const char* InverterAbstract::getChannelFieldName(uint8_t channel, uint8_t fieldId)
{
    uint8_t pos = getAssignIdxByChannelField(channel, fieldId);
    const byteAssign_t* b = getByteAssignment();

    return fields[b[pos].fieldId];
}

static float calcYieldTotalCh0(InverterAbstract* iv, uint8_t arg0)
{
    float yield = 0;
    for (uint8_t i = 1; i <= iv->getChannelCount(); i++) {
        yield += iv->getChannelFieldValue(i, FLD_YT);
    }
    return yield;
}

static float calcYieldDayCh0(InverterAbstract* iv, uint8_t arg0)
{
    float yield = 0;
    for (uint8_t i = 1; i <= iv->getChannelCount(); i++) {
        yield += iv->getChannelFieldValue(i, FLD_YD);
    }
    return yield;
}

// arg0 = channel of source
static float calcUdcCh(InverterAbstract* iv, uint8_t arg0)
{
    return iv->getChannelFieldValue(arg0, FLD_UDC);
}

static float calcPowerDcCh0(InverterAbstract* iv, uint8_t arg0)
{
    float dcPower = 0;
    for (uint8_t i = 1; i <= iv->getChannelCount(); i++) {
        dcPower += iv->getChannelFieldValue(i, FLD_PDC);
    }
    return dcPower;
}

// arg0 = channel
static float calcEffiencyCh0(InverterAbstract* iv, uint8_t arg0)
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
static float calcIrradiation(InverterAbstract* iv, uint8_t arg0)
{
    if (NULL != iv) {
        if (iv->getChannelMaxPower(arg0 - 1) > 0)
            return iv->getChannelFieldValue(arg0, FLD_PDC) / iv->getChannelMaxPower(arg0 - 1) * 100.0f;
    }
    return 0.0;
}