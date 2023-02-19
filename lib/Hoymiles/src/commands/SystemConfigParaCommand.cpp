// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "SystemConfigParaCommand.h"
#include "inverters/InverterAbstract.h"

SystemConfigParaCommand::SystemConfigParaCommand(uint64_t target_address, uint64_t router_address, time_t time)
    : MultiDataCommand(target_address, router_address)
{
    setTime(time);
    setDataType(0x05);
    setTimeout(200);
}

String SystemConfigParaCommand::getCommandName()
{
    return "SystemConfigPara";
}

bool SystemConfigParaCommand::handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id)
{
    // Check CRC of whole payload
    if (!MultiDataCommand::handleResponse(inverter, fragment, max_fragment_id)) {
        return false;
    }

    // Move all fragments into target buffer
    uint8_t offs = 0;
    inverter->SystemConfigPara()->clearBuffer();
    for (uint8_t i = 0; i < max_fragment_id; i++) {
        inverter->SystemConfigPara()->appendFragment(offs, fragment[i].fragment, fragment[i].len);
        offs += (fragment[i].len);
    }
    inverter->SystemConfigPara()->setLastUpdateRequest(millis());
    inverter->SystemConfigPara()->setLastLimitRequestSuccess(CMD_OK);
    return true;
}

void SystemConfigParaCommand::gotTimeout(InverterAbstract* inverter)
{
    inverter->SystemConfigPara()->setLastLimitRequestSuccess(CMD_NOK);
}