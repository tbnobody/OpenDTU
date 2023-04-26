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
#define MAX_ONLINE_FAILURE_COUNT 2

class CommandAbstract;

class InverterAbstract {
public:
    explicit InverterAbstract(HoymilesRadio* radio, uint64_t serial);
    void init();
    uint64_t serial();
    const String& serialString();
    void setName(const char* name);
    const char* name();
    virtual String typeName() = 0;
    virtual const byteAssign_t* getByteAssignment() = 0;
    virtual uint8_t getByteAssignmentSize() = 0;

    bool isProducing();
    bool isReachable();

    void setEnablePolling(bool enabled);
    bool getEnablePolling();

    void setEnableCommands(bool enabled);
    bool getEnableCommands();

    void clearRxFragmentBuffer();
    void addRxFragment(uint8_t fragment[], uint8_t len);
    uint8_t verifyAllFragments(CommandAbstract* cmd);

    virtual bool sendStatsRequest() = 0;
    virtual bool sendAlarmLogRequest(bool force = false) = 0;
    virtual bool sendDevInfoRequest() = 0;
    virtual bool sendSystemConfigParaRequest() = 0;
    virtual bool sendActivePowerControlRequest(float limit, PowerLimitControlType type) = 0;
    virtual bool resendActivePowerControlRequest() = 0;
    virtual bool sendPowerControlRequest(bool turnOn) = 0;
    virtual bool sendRestartControlRequest() = 0;
    virtual bool resendPowerControlRequest() = 0;
    virtual bool sendChangeChannelRequest();

    HoymilesRadio* getRadio();

    AlarmLogParser* EventLog();
    DevInfoParser* DevInfo();
    PowerCommandParser* PowerCommand();
    StatisticsParser* Statistics();
    SystemConfigParaParser* SystemConfigPara();

protected:
    HoymilesRadio* _radio;

private:
    serial_u _serial;
    String _serialString;
    char _name[MAX_NAME_LENGTH] = "";
    fragment_t _rxFragmentBuffer[MAX_RF_FRAGMENT_COUNT];
    uint8_t _rxFragmentMaxPacketId = 0;
    uint8_t _rxFragmentLastPacketId = 0;
    uint8_t _rxFragmentRetransmitCnt = 0;

    bool _enablePolling = true;
    bool _enableCommands = true;

    std::unique_ptr<AlarmLogParser> _alarmLogParser;
    std::unique_ptr<DevInfoParser> _devInfoParser;
    std::unique_ptr<PowerCommandParser> _powerCommandParser;
    std::unique_ptr<StatisticsParser> _statisticsParser;
    std::unique_ptr<SystemConfigParaParser> _systemConfigParaParser;
};