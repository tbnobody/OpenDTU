#pragma once

#include "../parser/AlarmLogParser.h"
#include "../parser/DevInfoParser.h"
#include "../parser/StatisticsParser.h"
#include "../parser/SystemConfigParaParser.h"
#include "HoymilesRadio.h"
#include "types.h"
#include <Arduino.h>
#include <cstdint>

#define MAX_NAME_LENGTH 32

enum {
    FRAGMENT_ALL_MISSING = 255,
    FRAGMENT_RETRANSMIT_TIMEOUT = 254,
    FRAGMENT_HANDLE_ERROR = 253,
    FRAGMENT_OK = 0
};

#define MAX_RF_FRAGMENT_COUNT 12
#define MAX_RETRANSMIT_COUNT 5

class CommandAbstract;

class InverterAbstract {
public:
    explicit InverterAbstract(uint64_t serial);
    void init();
    uint64_t serial();
    void setName(const char* name);
    const char* name();
    virtual String typeName() = 0;
    virtual const byteAssign_t* getByteAssignment() = 0;
    virtual const uint8_t getAssignmentCount() = 0;

    void clearRxFragmentBuffer();
    void addRxFragment(uint8_t fragment[], uint8_t len);
    uint8_t verifyAllFragments(CommandAbstract* cmd);

    virtual bool sendStatsRequest(HoymilesRadio* radio) = 0;
    virtual bool sendAlarmLogRequest(HoymilesRadio* radio) = 0;
    virtual bool sendDevInfoRequest(HoymilesRadio* radio) = 0;
    virtual bool sendSystemConfigParaRequest(HoymilesRadio* radio) = 0;

    AlarmLogParser* EventLog();
    DevInfoParser* DevInfo();
    StatisticsParser* Statistics();
    SystemConfigParaParser* SystemConfigPara();

private:
    serial_u _serial;
    char _name[MAX_NAME_LENGTH] = "";
    fragment_t _rxFragmentBuffer[MAX_RF_FRAGMENT_COUNT];
    uint8_t _rxFragmentMaxPacketId = 0;
    uint8_t _rxFragmentLastPacketId = 0;
    uint8_t _rxFragmentRetransmitCnt = 0;

    std::unique_ptr<AlarmLogParser> _alarmLogParser;
    std::unique_ptr<DevInfoParser> _devInfoParser;
    std::unique_ptr<StatisticsParser> _statisticsParser;
    std::unique_ptr<SystemConfigParaParser> _systemConfigParaParser;
};