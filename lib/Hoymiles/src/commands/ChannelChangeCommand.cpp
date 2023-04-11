// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "ChannelChangeCommand.h"

ChannelChangeCommand::ChannelChangeCommand(uint64_t target_address, uint64_t router_address, uint8_t channel)
    : CommandAbstract(target_address, router_address)
{
    _payload[0] = 0x56;
    _payload[9] = 0x02;
    _payload[10] = 0x15;
    _payload[11] = 0x21;
    _payload[13] = 0x14;
    _payload_size = 14;

    setChannel(channel);
    setTimeout(10);
}

String ChannelChangeCommand::getCommandName()
{
    return "ChannelChangeCommand";
}

void ChannelChangeCommand::setChannel(uint8_t channel)
{
    _payload[12] = channel;
}

uint8_t ChannelChangeCommand::getChannel()
{
    return _payload[12];
}

bool ChannelChangeCommand::handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id)
{
    return true;
}

uint8_t ChannelChangeCommand::getMaxResendCount()
{
    // This command will never retrieve an answer. Therefor it's not required to repeat it
    return 0;
}