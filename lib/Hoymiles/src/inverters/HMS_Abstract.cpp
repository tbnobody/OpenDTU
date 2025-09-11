// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2025 Thomas Basler and others
 */
#include "HMS_Abstract.h"
#include "Hoymiles.h"
#include "HoymilesRadio_CMT.h"
#include "commands/ChannelChangeCommand.h"

HMS_Abstract::HMS_Abstract(HoymilesRadio* radio, const uint64_t serial)
    : HM_Abstract(radio, serial)
{
}

bool HMS_Abstract::sendChangeChannelRequest()
{
    if (!(getEnableCommands() || getEnablePolling())) {
        return false;
    }

    auto cmdChannel = _radio->prepareCommand<ChannelChangeCommand>(this);
    cmdChannel->setCountryMode(Hoymiles.getRadioCmt()->getCountryMode());
    cmdChannel->setChannel(Hoymiles.getRadioCmt()->getChannelFromFrequency(Hoymiles.getRadioCmt()->getInverterTargetFrequency()));
    _radio->enqueCommand(cmdChannel);

    return true;
};

bool HMS_Abstract::sendActivePowerControlRequest(float limit, const PowerLimitControlType type)
{
    if (!getEnableCommands()) {
        return false;
    }

    if (type == PowerLimitControlType::RelativNonPersistent || type == PowerLimitControlType::RelativPersistent) {
        limit = min<float>(100, limit);
    }

    _activePowerControlLimit = limit;
    _activePowerControlType = type;

    auto cmd = _radio->prepareCommand<ActivePowerControlCommand>(this);
    cmd->setDeviceType(ActivePowerControlDeviceType::HmsActivePowerControl);
    cmd->setActivePowerLimit(limit, type);
    SystemConfigPara()->setLastLimitCommandSuccess(CMD_PENDING);
    _radio->enqueCommand(cmd);

    return true;
}
