// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "AlarmDataCommand.h"
#include "inverters/InverterAbstract.h"

AlarmDataCommand::AlarmDataCommand(uint64_t target_address, uint64_t router_address, time_t time)
    : MultiDataCommand(target_address, router_address)
{
    setTime(time);
    setDataType(0x11);
    setTimeout(750);
}

String AlarmDataCommand::getCommandName()
{
    return "AlarmData";
}

bool AlarmDataCommand::handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id)
{
    // Check CRC of whole payload
    if (!MultiDataCommand::handleResponse(inverter, fragment, max_fragment_id)) {
        return false;
    }

    // Move all fragments into target buffer
    uint8_t offs = 0;
    inverter->EventLog()->beginAppendFragment();
    inverter->EventLog()->clearBuffer();
    for (uint8_t i = 0; i < max_fragment_id; i++) {
        inverter->EventLog()->appendFragment(offs, fragment[i].fragment, fragment[i].len);
        offs += (fragment[i].len);
    }
    inverter->EventLog()->endAppendFragment();
    inverter->EventLog()->setLastAlarmRequestSuccess(CMD_OK);
    inverter->EventLog()->setLastUpdate(millis());
    return true;
}

void AlarmDataCommand::gotTimeout(InverterAbstract* inverter)
{
    inverter->EventLog()->setLastAlarmRequestSuccess(CMD_NOK);
}