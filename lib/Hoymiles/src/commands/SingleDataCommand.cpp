// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */

/*
This command is used to send simple commands, containing only one payload, to the inverter.

Derives from CommandAbstract.

Command structure:
* ID: fixed identifier and everytime 0x15

00   01 02 03 04   05 06 07 08   09   10   11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
---------------------------------------------------------------------------------------------------------
15   71 60 35 46   80 12 23 04   00   00   -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
^^   ^^^^^^^^^^^   ^^^^^^^^^^^   ^^   ^^
ID   Target Addr   Source Addr        CRC8
*/
#include "SingleDataCommand.h"

SingleDataCommand::SingleDataCommand(InverterAbstract* inv, const uint64_t router_address)
    : CommandAbstract(inv, router_address)
{
    _payload[0] = 0x15;
    setTimeout(100);
}
