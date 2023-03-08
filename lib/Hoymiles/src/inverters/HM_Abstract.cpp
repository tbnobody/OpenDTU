// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "HM_Abstract.h"
#include "HoymilesRadio.h"
#include "commands/ActivePowerControlCommand.h"
#include "commands/AlarmDataCommand.h"
#include "commands/DevInfoAllCommand.h"
#include "commands/DevInfoSimpleCommand.h"
#include "commands/PowerControlCommand.h"
#include "commands/RealTimeRunDataCommand.h"
#include "commands/SystemConfigParaCommand.h"

HM_Abstract::HM_Abstract(uint64_t serial)
    : InverterAbstract(serial) {};

bool HM_Abstract::sendStatsRequest(HoymilesRadio* radio)
{
    if (!getEnablePolling()) {
        return false;
    }

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        return false;
    }

    time_t now;
    time(&now);

    RealTimeRunDataCommand* cmd = radio->enqueCommand<RealTimeRunDataCommand>();
    cmd->setTime(now);
    cmd->setTargetAddress(serial());

    return true;
}

bool HM_Abstract::sendAlarmLogRequest(HoymilesRadio* radio, bool force)
{
    if (!getEnablePolling()) {
        return false;
    }

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        return false;
    }

    if (!force) {
        if (Statistics()->hasChannelFieldValue(TYPE_INV, CH0, FLD_EVT_LOG)) {
            if ((uint8_t)Statistics()->getChannelFieldValue(TYPE_INV, CH0, FLD_EVT_LOG) == _lastAlarmLogCnt) {
                return false;
            }
        }
    }

    _lastAlarmLogCnt = (uint8_t)Statistics()->getChannelFieldValue(TYPE_INV, CH0, FLD_EVT_LOG);

    time_t now;
    time(&now);

    AlarmDataCommand* cmd = radio->enqueCommand<AlarmDataCommand>();
    cmd->setTime(now);
    cmd->setTargetAddress(serial());
    EventLog()->setLastAlarmRequestSuccess(CMD_PENDING);

    return true;
}

bool HM_Abstract::sendDevInfoRequest(HoymilesRadio* radio)
{
    if (!getEnablePolling()) {
        return false;
    }

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        return false;
    }

    time_t now;
    time(&now);

    DevInfoAllCommand* cmdAll = radio->enqueCommand<DevInfoAllCommand>();
    cmdAll->setTime(now);
    cmdAll->setTargetAddress(serial());

    DevInfoSimpleCommand* cmdSimple = radio->enqueCommand<DevInfoSimpleCommand>();
    cmdSimple->setTime(now);
    cmdSimple->setTargetAddress(serial());

    return true;
}

bool HM_Abstract::sendSystemConfigParaRequest(HoymilesRadio* radio)
{
    if (!getEnablePolling()) {
        return false;
    }

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        return false;
    }

    time_t now;
    time(&now);

    SystemConfigParaCommand* cmd = radio->enqueCommand<SystemConfigParaCommand>();
    cmd->setTime(now);
    cmd->setTargetAddress(serial());
    SystemConfigPara()->setLastLimitRequestSuccess(CMD_PENDING);

    return true;
}

bool HM_Abstract::sendActivePowerControlRequest(HoymilesRadio* radio, float limit, PowerLimitControlType type)
{
    if (!getEnableCommands()) {
        return false;
    }

    if (type == PowerLimitControlType::RelativNonPersistent || type == PowerLimitControlType::RelativPersistent) {
        limit = min<float>(100, limit);
    }

    _activePowerControlLimit = limit;
    _activePowerControlType = type;

    ActivePowerControlCommand* cmd = radio->enqueCommand<ActivePowerControlCommand>();
    cmd->setActivePowerLimit(limit, type);
    cmd->setTargetAddress(serial());
    SystemConfigPara()->setLastLimitCommandSuccess(CMD_PENDING);

    return true;
}

bool HM_Abstract::resendActivePowerControlRequest(HoymilesRadio* radio)
{
    return sendActivePowerControlRequest(radio, _activePowerControlLimit, _activePowerControlType);
}

bool HM_Abstract::sendPowerControlRequest(HoymilesRadio* radio, bool turnOn)
{
    if (!getEnableCommands()) {
        return false;
    }

    if (turnOn) {
        _powerState = 1;
    } else {
        _powerState = 0;
    }

    PowerControlCommand* cmd = radio->enqueCommand<PowerControlCommand>();
    cmd->setPowerOn(turnOn);
    cmd->setTargetAddress(serial());
    PowerCommand()->setLastPowerCommandSuccess(CMD_PENDING);

    return true;
}

bool HM_Abstract::sendRestartControlRequest(HoymilesRadio* radio)
{
    if (!getEnableCommands()) {
        return false;
    }

    _powerState = 2;

    PowerControlCommand* cmd = radio->enqueCommand<PowerControlCommand>();
    cmd->setRestart();
    cmd->setTargetAddress(serial());
    PowerCommand()->setLastPowerCommandSuccess(CMD_PENDING);

    return true;
}

bool HM_Abstract::resendPowerControlRequest(HoymilesRadio* radio)
{
    switch (_powerState) {
    case 0:
        return sendPowerControlRequest(radio, false);
        break;
    case 1:
        return sendPowerControlRequest(radio, true);
        break;
    case 2:
        return sendRestartControlRequest(radio);
        break;

    default:
        return false;
        break;
    }
}