// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2026 Thomas Basler and others
 */

/*
Derives from CommandAbstract. Special command to set frequency channel on HMS/HMT inverters.

Command structure:
* ID: fixed identifier and everytime 0x56
* CH: Channel to which the inverter will be switched to

00   01 02 03 04   05 06 07 08   09   10   11   12   13   14   15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
-----------------------------------------------------------------------------------------------------------------
56   71 60 35 46   80 12 23 04   02   15   21   00   14   00   -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- (860 MHz band)
56   71 60 35 46   80 12 23 04   03   17   3c   00   14   00   -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- (900 MHz band)
^^   ^^^^^^^^^^^   ^^^^^^^^^^^   ^^   ^^   ^^   ^^   ^^   ^^
ID   Target Addr   Source Addr   ?    ?    ?    CH   ?    CRC8
*/
#include "ChannelChangeCommand.h"
#include "../CmtLinkRecovery.h"
#include "inverters/InverterAbstract.h"

ChannelChangeCommand::ChannelChangeCommand(InverterAbstract* inv, const uint64_t router_address, const uint8_t channel)
    : CommandAbstract(inv, router_address)
{
    _payload[0] = 0x56;
    _payload[13] = 0x14;
    _payload_size = 14;

    setCountryMode(CountryModeId_t::MODE_EU);
    setChannel(channel);

    _singleFragmentAnswer = true;
    setTimeout(150);
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

void ChannelChangeCommand::setCountryMode(const CountryModeId_t mode)
{
    switch (mode) {
    case CountryModeId_t::MODE_US:
        _payload[9] = 0x03;
        _payload[10] = 0x17;
        _payload[11] = 0x3c;
        break;
    case CountryModeId_t::MODE_BR:
        _payload[9] = 0x03;
        _payload[10] = 0x17;
        _payload[11] = 0x3c;
        break;
    default:
        _payload[9] = 0x02;
        _payload[10] = 0x15;
        _payload[11] = 0x21;
        break;
    }
}

void ChannelChangeCommand::setBandBytes(const uint8_t profile, const uint8_t startChannel)
{
    if (profile != 0 && startChannel != 0) {
        _payload[10] = profile;
        _payload[11] = startChannel;
    }
}

/*
Answer of the RF module (mainCmd 0xD6), payload after the address header:

00      01       02
Status  Profile  Start channel

Status 0: channel change accepted, the module switches after sending this answer.
Status 1: profile or start channel of the request do not match the module's, nothing was changed. Profile and
start channel are the module's real values.
*/
bool ChannelChangeCommand::handleResponse(const fragment_t fragment[], const uint8_t max_fragment_id)
{
    if (max_fragment_id == 0) {
        return false;
    }

    const fragment_t& f = fragment[max_fragment_id - 1];

    if (f.mainCmd != 0xD6 || f.len < 3) {
        return false;
    }

    const uint8_t status = f.fragment[0];
    const uint8_t profile = f.fragment[1];
    const uint8_t startChannel = f.fragment[2];

    if (status == 0) {
        CmtLinkRecovery::onChannelChangeAccepted(_inv, profile, startChannel);
    } else {
        CmtLinkRecovery::onChannelChangeRejected(_inv, profile, startChannel);
    }

    return true;
}

uint8_t ChannelChangeCommand::getMaxResendCount() const
{
    // The next polling cycle sends the command again, an immediate repetition does not help
    return 0;
}

bool ChannelChangeCommand::areSameParameter(CommandAbstract* other)
{
    if (!CommandAbstract::areSameParameter(other)) {
        return false;
    }
    const uint8_t* otherPayload = other->getDataPayload();
    return _payload[10] == otherPayload[10]
        && _payload[11] == otherPayload[11]
        && _payload[12] == otherPayload[12];
}
