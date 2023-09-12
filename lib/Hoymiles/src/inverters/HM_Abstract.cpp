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
#include "commands/GridOnProFilePara.h"
#include "commands/PowerControlCommand.h"
#include "commands/RealTimeRunDataCommand.h"
#include "commands/SystemConfigParaCommand.h"

HM_Abstract::HM_Abstract(HoymilesRadio* radio, uint64_t serial)
    : InverterAbstract(radio, serial) {};

bool HM_Abstract::sendStatsRequest()
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

    auto cmd = _radio->prepareCommand<RealTimeRunDataCommand>();
    cmd->setTime(now);
    cmd->setTargetAddress(serial());
    _radio->enqueCommand(cmd);

    return true;
}

bool HM_Abstract::sendAlarmLogRequest(bool force)
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

    auto cmd = _radio->prepareCommand<AlarmDataCommand>();
    cmd->setTime(now);
    cmd->setTargetAddress(serial());
    EventLog()->setLastAlarmRequestSuccess(CMD_PENDING);
    _radio->enqueCommand(cmd);

    return true;
}

bool HM_Abstract::sendDevInfoRequest()
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

    auto cmdAll = _radio->prepareCommand<DevInfoAllCommand>();
    cmdAll->setTime(now);
    cmdAll->setTargetAddress(serial());
    _radio->enqueCommand(cmdAll);

    auto cmdSimple = _radio->prepareCommand<DevInfoSimpleCommand>();
    cmdSimple->setTime(now);
    cmdSimple->setTargetAddress(serial());
    _radio->enqueCommand(cmdSimple);

    return true;
}

bool HM_Abstract::sendSystemConfigParaRequest()
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

    auto cmd = _radio->prepareCommand<SystemConfigParaCommand>();
    cmd->setTime(now);
    cmd->setTargetAddress(serial());
    SystemConfigPara()->setLastLimitRequestSuccess(CMD_PENDING);
    _radio->enqueCommand(cmd);

    return true;
}

bool HM_Abstract::sendActivePowerControlRequest(float limit, PowerLimitControlType type)
{
    if (!getEnableCommands()) {
        return false;
    }

    if (type == PowerLimitControlType::RelativNonPersistent || type == PowerLimitControlType::RelativPersistent) {
        limit = min<float>(100, limit);
    }

    _activePowerControlLimit = limit;
    _activePowerControlType = type;

    auto cmd = _radio->prepareCommand<ActivePowerControlCommand>();
    cmd->setActivePowerLimit(limit, type);
    cmd->setTargetAddress(serial());
    SystemConfigPara()->setLastLimitCommandSuccess(CMD_PENDING);
    _radio->enqueCommand(cmd);

    return true;
}

bool HM_Abstract::resendActivePowerControlRequest()
{
    return sendActivePowerControlRequest(_activePowerControlLimit, _activePowerControlType);
}

bool HM_Abstract::sendPowerControlRequest(bool turnOn)
{
    if (!getEnableCommands()) {
        return false;
    }

    if (turnOn) {
        _powerState = 1;
    } else {
        _powerState = 0;
    }

    auto cmd = _radio->prepareCommand<PowerControlCommand>();
    cmd->setPowerOn(turnOn);
    cmd->setTargetAddress(serial());
    PowerCommand()->setLastPowerCommandSuccess(CMD_PENDING);
    _radio->enqueCommand(cmd);

    return true;
}

bool HM_Abstract::sendRestartControlRequest()
{
    if (!getEnableCommands()) {
        return false;
    }

    _powerState = 2;

    auto cmd = _radio->prepareCommand<PowerControlCommand>();
    cmd->setRestart();
    cmd->setTargetAddress(serial());
    PowerCommand()->setLastPowerCommandSuccess(CMD_PENDING);
    _radio->enqueCommand(cmd);

    return true;
}

bool HM_Abstract::resendPowerControlRequest()
{
    switch (_powerState) {
    case 0:
        return sendPowerControlRequest(false);
        break;
    case 1:
        return sendPowerControlRequest(true);
        break;
    case 2:
        return sendRestartControlRequest();
        break;

    default:
        return false;
        break;
    }
}

bool HM_Abstract::sendGridOnProFileParaRequest()
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

    auto cmd = _radio->prepareCommand<GridOnProFilePara>();
    cmd->setTime(now);
    cmd->setTargetAddress(serial());
    _radio->enqueCommand(cmd);

    return true;
}
