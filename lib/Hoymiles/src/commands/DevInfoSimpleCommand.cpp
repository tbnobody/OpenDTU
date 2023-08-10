// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "DevInfoSimpleCommand.h"
#include "inverters/InverterAbstract.h"

DevInfoSimpleCommand::DevInfoSimpleCommand(uint64_t target_address, uint64_t router_address, time_t time)
    : MultiDataCommand(target_address, router_address)
{
    setTime(time);
    setDataType(0x00);
    setTimeout(200);
}

String DevInfoSimpleCommand::getCommandName()
{
    return "DevInfoSimple";
}

bool DevInfoSimpleCommand::handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id)
{
    // Check CRC of whole payload
    if (!MultiDataCommand::handleResponse(inverter, fragment, max_fragment_id)) {
        return false;
    }

    // Move all fragments into target buffer
    uint8_t offs = 0;
    inverter->DevInfo()->beginAppendFragment();
    inverter->DevInfo()->clearBufferSimple();
    for (uint8_t i = 0; i < max_fragment_id; i++) {
        inverter->DevInfo()->appendFragmentSimple(offs, fragment[i].fragment, fragment[i].len);
        offs += (fragment[i].len);
    }
    inverter->DevInfo()->endAppendFragment();
    inverter->DevInfo()->setLastUpdateSimple(millis());
    return true;
}