// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "DevInfoAllCommand.h"
#include "inverters/InverterAbstract.h"

DevInfoAllCommand::DevInfoAllCommand(uint64_t target_address, uint64_t router_address, time_t time)
    : MultiDataCommand(target_address, router_address)
{
    setTime(time);
    setDataType(0x01);
    setTimeout(200);
}

String DevInfoAllCommand::getCommandName()
{
    return "DevInfoAll";
}

bool DevInfoAllCommand::handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id)
{
    // Check CRC of whole payload
    if (!MultiDataCommand::handleResponse(inverter, fragment, max_fragment_id)) {
        return false;
    }

    // Move all fragments into target buffer
    uint8_t offs = 0;
    inverter->DevInfo()->clearBufferAll();
    for (uint8_t i = 0; i < max_fragment_id; i++) {
        inverter->DevInfo()->appendFragmentAll(offs, fragment[i].fragment, fragment[i].len);
        offs += (fragment[i].len);
    }
    inverter->DevInfo()->setLastUpdateAll(millis());
    return true;
}