// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2026 Thomas Basler and others
 */

/*
This command is used to fetch the hardware and firmware version of the
inverter's built-in RF module.

Derives from CommandAbstract. Has a fixed length of 9 bytes.

Command structure:
* ID: fixed identifier and everytime 0x06

00   01 02 03 04   05 06 07 08   09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
-------------------------------------------------------------------------------------------------------
06   71 60 35 46   80 12 23 04   -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
^^   ^^^^^^^^^^^   ^^^^^^^^^^^
ID   Target Addr   Source Addr

The response is a single fragment answer (mainCmd 0x86) whose payload -
already stripped of the address header by the radio layer - looks like this:

00 01   02 03 04 05   06 07 08 09    10 11 12 13
-------------------------------------------------
PID     Serial        RFHW Version   RFFW Version

Observed in the wild: the fragment is not numbered 1 and does not carry the
usual 0x80 "last fragment" flag (e.g. its fragment header byte is 0x02
instead of 0x81). _singleFragmentAnswer makes the generic reassembly accept
it anyway, so the answer ends up in the highest numbered fragment slot
rather than fragment[0].

Note that the answer is therefore still subject to the generic fragment
handling in InverterAbstract::addRxFragment(), which interprets byte 09 of
the answer as a fragment id. Should an inverter ever put 0x00 or a value
>= MAX_RF_FRAGMENT_COUNT there, the answer is dropped before it reaches
this command.
*/
#include "RfInfoCommand.h"
#include "inverters/InverterAbstract.h"

// Command id, and the flag the inverter sets on it to mark its answer
constexpr uint8_t CommandId = 0x06;
constexpr uint8_t ResponseFlag = 0x80;

RfInfoCommand::RfInfoCommand(InverterAbstract* inv, const uint64_t router_address)
    : CommandAbstract(inv, router_address)
{
    _payload[0] = CommandId;
    _payload_size = 9;
    _singleFragmentAnswer = true;

    setTimeout(200);
}

String RfInfoCommand::getCommandName() const
{
    return "RfInfo";
}

bool RfInfoCommand::handleResponse(const fragment_t fragment[], const uint8_t max_fragment_id)
{
    if (max_fragment_id == 0) {
        return false;
    }

    const fragment_t& f = fragment[max_fragment_id - 1];

    if (f.mainCmd != (CommandId | ResponseFlag)) {
        return false;
    }

    if (!_inv->RfInfo()->setPayload(f.fragment, f.len)) {
        return false;
    }

    _inv->RfInfo()->setLastUpdate(millis());

    return true;
}
