// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "../commands/ActivePowerControlCommand.h"
#include "../parser/AlarmLogParser.h"
#include "../parser/DevInfoParser.h"
#include "../parser/GridProfileParser.h"
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

class CommandAbstract;

class InverterAbstract {
public:
    explicit InverterAbstract(HoymilesRadio* radio, const uint64_t serial);
    void init();
    uint64_t serial() const;
    const String& serialString() const;
    void setName(const char* name);
    const char* name() const;
    virtual String typeName() const = 0;
    virtual const byteAssign_t* getByteAssignment() const = 0;
    virtual uint8_t getByteAssignmentSize() const = 0;

    bool isProducing();
    bool isReachable();

    void setEnablePolling(const bool enabled);
    bool getEnablePolling() const;

    void setEnableCommands(const bool enabled);
    bool getEnableCommands() const;

    void setReachableThreshold(const uint8_t threshold);
    uint8_t getReachableThreshold() const;

    void setZeroValuesIfUnreachable(const bool enabled);
    bool getZeroValuesIfUnreachable() const;

    void setZeroYieldDayOnMidnight(const bool enabled);
    bool getZeroYieldDayOnMidnight() const;

    void setClearEventlogOnMidnight(const bool enabled);
    bool getClearEventlogOnMidnight() const;

    int8_t getLastRssi() const;

    void clearRxFragmentBuffer();
    void addRxFragment(const uint8_t fragment[], const uint8_t len, const int8_t rssi);
    uint8_t verifyAllFragments(CommandAbstract& cmd);

    void performDailyTask();

    void resetRadioStats();

    struct {
        // TX Request Data
        uint32_t TxRequestData;

        // TX Re-Request Fragment
        uint32_t TxReRequestFragment;

        // RX Success
        uint32_t RxSuccess;

        // RX Fail Partial Answer
        uint32_t RxFailPartialAnswer;

        // RX Fail No Answer
        uint32_t RxFailNoAnswer;

        // RX Fail Corrupt Data
        uint32_t RxFailCorruptData;
    } RadioStats = {};

    virtual bool sendStatsRequest() = 0;
    virtual bool sendAlarmLogRequest(const bool force = false) = 0;
    virtual bool sendDevInfoRequest() = 0;
    virtual bool sendSystemConfigParaRequest() = 0;
    virtual bool sendActivePowerControlRequest(float limit, const PowerLimitControlType type) = 0;
    virtual bool resendActivePowerControlRequest() = 0;
    virtual bool sendPowerControlRequest(const bool turnOn) = 0;
    virtual bool sendRestartControlRequest() = 0;
    virtual bool resendPowerControlRequest() = 0;
    virtual bool sendChangeChannelRequest();
    virtual bool sendGridOnProFileParaRequest() = 0;

    HoymilesRadio* getRadio();

    AlarmLogParser* EventLog();
    DevInfoParser* DevInfo();
    GridProfileParser* GridProfile();
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

    uint8_t _reachableThreshold = 3;

    bool _zeroValuesIfUnreachable = false;
    bool _zeroYieldDayOnMidnight = false;
    bool _clearEventlogOnMidnight = false;

    int8_t _lastRssi = -127;

    std::unique_ptr<AlarmLogParser> _alarmLogParser;
    std::unique_ptr<DevInfoParser> _devInfoParser;
    std::unique_ptr<GridProfileParser> _gridProfileParser;
    std::unique_ptr<PowerCommandParser> _powerCommandParser;
    std::unique_ptr<StatisticsParser> _statisticsParser;
    std::unique_ptr<SystemConfigParaParser> _systemConfigParaParser;
};
