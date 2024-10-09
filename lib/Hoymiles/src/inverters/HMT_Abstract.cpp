// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2024 Thomas Basler and others
 */
#include "HMT_Abstract.h"
#include "Hoymiles.h"
#include "HoymilesRadio_CMT.h"
#include "commands/ChannelChangeCommand.h"
#include "parser/AlarmLogParser.h"

HMT_Abstract::HMT_Abstract(HoymilesRadio* radio, const uint64_t serial)
    : HM_Abstract(radio, serial)
{
    EventLog()->setMessageType(AlarmMessageType_t::HMT);
};

bool HMT_Abstract::sendChangeChannelRequest()
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
