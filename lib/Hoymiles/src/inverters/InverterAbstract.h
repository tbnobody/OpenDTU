#pragma once

#include "../parser/AlarmLogParser.h"
#include "../parser/StatisticsParser.h"
#include "HoymilesRadio.h"
#include "types.h"
#include <Arduino.h>
#include <cstdint>

#define MAX_NAME_LENGTH 32

enum {
    FRAGMENT_ALL_MISSING = 255,
    FRAGMENT_RETRANSMIT_TIMEOUT = 254,
    FRAGMENT_CRC_ERROR = 253,
    FRAGMENT_OK = 0
};

#define MAX_RF_FRAGMENT_COUNT 5
#define MAX_RETRANSMIT_COUNT 5

class InverterAbstract {
public:
    InverterAbstract(uint64_t serial);
    void init();
    uint64_t serial();
    void setName(const char* name);
    const char* name();
    virtual String typeName() = 0;
    virtual const byteAssign_t* getByteAssignment() = 0;
    virtual const uint8_t getAssignmentCount() = 0;

    void clearRxFragmentBuffer();
    void addRxFragment(uint8_t fragment[], uint8_t len);
    uint8_t verifyAllFragments();

    virtual bool sendStatsRequest(HoymilesRadio* radio) = 0;
    virtual bool sendAlarmLogRequest(HoymilesRadio* radio) = 0;
    uint32_t getLastStatsUpdate();

    void setLastRequest(RequestType request);

    AlarmLogParser* EventLog();
    StatisticsParser* Statistics();

protected:
    RequestType getLastRequest();

private:
    serial_u _serial;
    char _name[MAX_NAME_LENGTH];
    fragment_t _rxFragmentBuffer[MAX_RF_FRAGMENT_COUNT];
    uint8_t _rxFragmentMaxPacketId = 0;
    uint8_t _rxFragmentLastPacketId = 0;
    uint8_t _rxFragmentRetransmitCnt = 0;

    uint32_t _lastStatsUpdate = 0;
    uint32_t _lastAlarmLogUpdate = 0;

    RequestType _lastRequest = RequestType::None;

    std::unique_ptr<AlarmLogParser> _alarmLogParser;
    std::unique_ptr<StatisticsParser> _statisticsParser;
};