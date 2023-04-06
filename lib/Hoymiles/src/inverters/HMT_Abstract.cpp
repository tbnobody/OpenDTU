// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "HMT_Abstract.h"
#include "Hoymiles.h"
#include "HoymilesRadio_CMT.h"
#include "commands/ChannelChangeCommand.h"
#include "parser/AlarmLogParser.h"

HMT_Abstract::HMT_Abstract(HoymilesRadio* radio, uint64_t serial)
    : HM_Abstract(radio, serial)
{
    EventLog()->setMessageType(AlarmMessageType_t::HMT);
};

bool HMT_Abstract::sendChangeChannelRequest()
{
    if (!(getEnableCommands() && getEnablePolling())) {
        return false;
    }

    ChannelChangeCommand* cmdChannel = _radio->enqueCommand<ChannelChangeCommand>();
    cmdChannel->setChannel(HoymilesRadio_CMT::getChannelFromFrequency(Hoymiles.getRadioCmt()->getInverterTargetFrequency()));
    cmdChannel->setTargetAddress(serial());

    return true;
};