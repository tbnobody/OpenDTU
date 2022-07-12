#include "HM_Abstract.h"
#include "HoymilesRadio.h"
#include "crc.h"

HM_Abstract::HM_Abstract(uint64_t serial)
    : InverterAbstract(serial) {};

bool HM_Abstract::sendStatsRequest(HoymilesRadio* radio)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return false;
    }

    time_t now;
    time(&now);

    inverter_transaction_t payload;

    memset(payload.payload, 0, MAX_RF_PAYLOAD_SIZE);

    payload.target.u64 = serial();
    payload.mainCmd = 0x15;
    payload.subCmd = 0x80;
    payload.timeout = 200;
    payload.len = 16;

    payload.payload[0] = 0x0b;
    payload.payload[1] = 0x00;

    HoymilesRadio::u32CpyLittleEndian(&payload.payload[2], now); // sets the 4 following elements {2, 3, 4, 5}
    payload.payload[9] = 0x05;

    uint16_t crc = crc16(&payload.payload[0], 14);
    payload.payload[14] = (crc >> 8) & 0xff;
    payload.payload[15] = (crc)&0xff;

    payload.requestType = RequestType::Stats;

    radio->enqueTransaction(&payload);
    return true;
}

bool HM_Abstract::sendAlarmLogRequest(HoymilesRadio* radio)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return false;
    }

    if (Statistics()->hasChannelFieldValue(CH0, FLD_EVT_LOG)) {
        if ((uint8_t)Statistics()->getChannelFieldValue(CH0, FLD_EVT_LOG) == _lastAlarmLogCnt) {
            return false;
        }
    }

    _lastAlarmLogCnt = (uint8_t)Statistics()->getChannelFieldValue(CH0, FLD_EVT_LOG);

    time_t now;
    time(&now);

    inverter_transaction_t payload;

    memset(payload.payload, 0, MAX_RF_PAYLOAD_SIZE);

    payload.target.u64 = serial();
    payload.mainCmd = 0x15;
    payload.subCmd = 0x80;
    payload.timeout = 200;
    payload.len = 16;

    payload.payload[0] = 0x11;
    payload.payload[1] = 0x00;

    HoymilesRadio::u32CpyLittleEndian(&payload.payload[2], now); // sets the 4 following elements {2, 3, 4, 5}

    uint16_t crc = crc16(&payload.payload[0], 14);
    payload.payload[14] = (crc >> 8) & 0xff;
    payload.payload[15] = (crc)&0xff;

    payload.requestType = RequestType::AlarmLog;

    radio->enqueTransaction(&payload);
    return true;
}