// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
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

    auto cmdChannel = _radio->prepareCommand<ChannelChangeCommand>();
    cmdChannel->setChannel(HoymilesRadio_CMT::getChannelFromFrequency(Hoymiles.getRadioCmt()->getInverterTargetFrequency()));
    cmdChannel->setTargetAddress(serial());
    _radio->enqueCommand(cmdChannel);

    return true;
};
