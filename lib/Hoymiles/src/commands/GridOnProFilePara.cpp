// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "GridOnProFilePara.h"
#include "Hoymiles.h"
#include "inverters/InverterAbstract.h"

GridOnProFilePara::GridOnProFilePara(uint64_t target_address, uint64_t router_address, time_t time)
    : MultiDataCommand(target_address, router_address)
{
    setTime(time);
    setDataType(0x02);
    setTimeout(500);
}

String GridOnProFilePara::getCommandName()
{
    return "GridOnProFilePara";
}

bool GridOnProFilePara::handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id)
{
    // Check CRC of whole payload
    if (!MultiDataCommand::handleResponse(inverter, fragment, max_fragment_id)) {
        return false;
    }

    // Move all fragments into target buffer
    uint8_t offs = 0;
    inverter->GridProfile()->beginAppendFragment();
    inverter->GridProfile()->clearBuffer();
    for (uint8_t i = 0; i < max_fragment_id; i++) {
        inverter->GridProfile()->appendFragment(offs, fragment[i].fragment, fragment[i].len);
        offs += (fragment[i].len);
    }
    inverter->GridProfile()->endAppendFragment();
    inverter->GridProfile()->setLastUpdate(millis());
    return true;
}