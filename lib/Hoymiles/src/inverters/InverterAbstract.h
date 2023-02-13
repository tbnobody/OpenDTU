// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "../commands/ActivePowerControlCommand.h"
#include "../parser/AlarmLogParser.h"
#include "../parser/DevInfoParser.h"
#include "../parser/PowerCommandParser.h"
#include "../parser/StatisticsParser.h"
#include "../parser/SystemConfigParaParser.h"
#include "HoymilesRadio.h"
#include "types.h"
#include <Arduino.h>
#include <cstdint>
#include <list>

#define MAX_NAME_LENGTH 32

enum {
    FRAGMENT_ALL_MISSING_RESEND = 255,
    FRAGMENT_ALL_MISSING_TIMEOUT = 254,
    FRAGMENT_RETRANSMIT_TIMEOUT = 253,
    FRAGMENT_HANDLE_ERROR = 252,
    FRAGMENT_OK = 0
};

#define MAX_RF_FRAGMENT_COUNT 13
#define MAX_RETRANSMIT_COUNT 5 // Used to send the retransmit package
#define MAX_RESEND_COUNT 4 // Used if all packages are missing
#define MAX_ONLINE_FAILURE_COUNT 2

class CommandAbstract;

class InverterAbstract {
public:
    explicit InverterAbstract(uint64_t serial);
    void init();
    uint64_t serial();
    const String& serialString();
    void setName(const char* name);
    const char* name();
    virtual String typeName() = 0;
    virtual const std::list<byteAssign_t>* getByteAssignment() = 0;

    bool isProducing();
    bool isReachable();

    void clearRxFragmentBuffer();
    void addRxFragment(uint8_t fragment[], uint8_t len);
    uint8_t verifyAllFragments(CommandAbstract* cmd);

    virtual bool sendStatsRequest(HoymilesRadio* radio) = 0;
    virtual bool sendAlarmLogRequest(HoymilesRadio* radio, bool force = false) = 0;
    virtual bool sendDevInfoRequest(HoymilesRadio* radio) = 0;
    virtual bool sendSystemConfigParaRequest(HoymilesRadio* radio) = 0;
    virtual bool sendActivePowerControlRequest(HoymilesRadio* radio, float limit, PowerLimitControlType type) = 0;
    virtual bool resendActivePowerControlRequest(HoymilesRadio* radio) = 0;
    virtual bool sendPowerControlRequest(HoymilesRadio* radio, bool turnOn) = 0;
    virtual bool sendRestartControlRequest(HoymilesRadio* radio) = 0;
    virtual bool resendPowerControlRequest(HoymilesRadio* radio) = 0;

    AlarmLogParser* EventLog();
    DevInfoParser* DevInfo();
    PowerCommandParser* PowerCommand();
    StatisticsParser* Statistics();
    SystemConfigParaParser* SystemConfigPara();

private:
    serial_u _serial;
    String _serialString;
    char _name[MAX_NAME_LENGTH] = "";
    fragment_t _rxFragmentBuffer[MAX_RF_FRAGMENT_COUNT];
    uint8_t _rxFragmentMaxPacketId = 0;
    uint8_t _rxFragmentLastPacketId = 0;
    uint8_t _rxFragmentRetransmitCnt = 0;

    std::unique_ptr<AlarmLogParser> _alarmLogParser;
    std::unique_ptr<DevInfoParser> _devInfoParser;
    std::unique_ptr<PowerCommandParser> _powerCommandParser;
    std::unique_ptr<StatisticsParser> _statisticsParser;
    std::unique_ptr<SystemConfigParaParser> _systemConfigParaParser;
};