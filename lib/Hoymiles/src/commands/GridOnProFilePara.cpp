// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */

/*
This command is used to fetch the grid profile from the inverter.

Derives from MultiDataCommand

Command structure:
* DT: this specific command uses 0x02

00   01 02 03 04   05 06 07 08   09   10   11   12 13 14 15   16 17   18 19   20 21 22 23   24 25   26   27 28 29 30 31
-----------------------------------------------------------------------------------------------------------------------
                                      |<------------------- CRC16 --------------------->|
15   71 60 35 46   80 12 23 04   80   02   00   65 72 06 B8   00 00   00 00   00 00 00 00   00 00   00   -- -- -- -- --
^^   ^^^^^^^^^^^   ^^^^^^^^^^^   ^^   ^^   ^^   ^^^^^^^^^^^   ^^^^^           ^^^^^^^^^^^   ^^^^^   ^^
ID   Target Addr   Source Addr   Idx  DT   ?    Time          Gap             Password      CRC16   CRC8
*/
#include "GridOnProFilePara.h"
#include "Hoymiles.h"
#include "inverters/InverterAbstract.h"

GridOnProFilePara::GridOnProFilePara(const uint64_t target_address, const uint64_t router_address, const time_t time)
    : MultiDataCommand(target_address, router_address)
{
    setTime(time);
    setDataType(0x02);
    setTimeout(500);
}

String GridOnProFilePara::getCommandName() const
{
    return "GridOnProFilePara";
}

bool GridOnProFilePara::handleResponse(InverterAbstract& inverter, const fragment_t fragment[], const uint8_t max_fragment_id)
{
    // Check CRC of whole payload
    if (!MultiDataCommand::handleResponse(inverter, fragment, max_fragment_id)) {
        return false;
    }

    // Move all fragments into target buffer
    uint8_t offs = 0;
    inverter.GridProfile()->beginAppendFragment();
    inverter.GridProfile()->clearBuffer();
    for (uint8_t i = 0; i < max_fragment_id; i++) {
        inverter.GridProfile()->appendFragment(offs, fragment[i].fragment, fragment[i].len);
        offs += (fragment[i].len);
    }
    inverter.GridProfile()->endAppendFragment();
    inverter.GridProfile()->setLastUpdate(millis());
    return true;
}