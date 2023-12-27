// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */

/*
Derives from CommandAbstract. Special command to set frequency channel on HMS/HMT inverters.

Command structure:
* ID: fixed identifier and everytime 0x56
* CH: Channel to which the inverter will be switched to

00   01 02 03 04   05 06 07 08   09   10   11   12   13   14   15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
-----------------------------------------------------------------------------------------------------------------
56   71 60 35 46   80 12 23 04   02   15   21   00   14   00   -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
^^   ^^^^^^^^^^^   ^^^^^^^^^^^   ^^   ^^   ^^   ^^   ^^   ^^
ID   Target Addr   Source Addr   ?    ?    ?    CH   ?    CRC8
*/
#include "ChannelChangeCommand.h"

ChannelChangeCommand::ChannelChangeCommand(const uint64_t target_address, const uint64_t router_address, const uint8_t channel)
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

String ChannelChangeCommand::getCommandName() const
{
    return "ChannelChangeCommand";
}

void ChannelChangeCommand::setChannel(const uint8_t channel)
{
    _payload[12] = channel;
}

uint8_t ChannelChangeCommand::getChannel() const
{
    return _payload[12];
}

bool ChannelChangeCommand::handleResponse(InverterAbstract& inverter, const fragment_t fragment[], const uint8_t max_fragment_id)
{
    return true;
}

uint8_t ChannelChangeCommand::getMaxResendCount()
{
    // This command will never retrieve an answer. Therefor it's not required to repeat it
    return 0;
}