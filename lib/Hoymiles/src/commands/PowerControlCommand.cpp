// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "PowerControlCommand.h"
#include "inverters/InverterAbstract.h"

#define CRC_SIZE 2

PowerControlCommand::PowerControlCommand(uint64_t target_address, uint64_t router_address)
    : DevControlCommand(target_address, router_address)
{
    _payload[10] = 0x00; // TurnOn
    _payload[11] = 0x00;

    udpateCRC(CRC_SIZE); // 2 byte crc

    _payload_size = 14;

    setTimeout(2000);
}

String PowerControlCommand::getCommandName()
{
    return "PowerControl";
}

bool PowerControlCommand::handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id)
{
    if (!DevControlCommand::handleResponse(inverter, fragment, max_fragment_id)) {
        return false;
    }

    inverter->PowerCommand()->setLastUpdateCommand(millis());
    inverter->PowerCommand()->setLastPowerCommandSuccess(CMD_OK);
    return true;
}

void PowerControlCommand::gotTimeout(InverterAbstract* inverter)
{
    inverter->PowerCommand()->setLastPowerCommandSuccess(CMD_NOK);
}

void PowerControlCommand::setPowerOn(bool state)
{
    if (state) {
        _payload[10] = 0x00; // TurnOn
    } else {
        _payload[10] = 0x01; // TurnOff
    }

    udpateCRC(CRC_SIZE); // 2 byte crc
}

void PowerControlCommand::setRestart()
{
    _payload[10] = 0x02; // Restart

    udpateCRC(CRC_SIZE); // 2 byte crc
}