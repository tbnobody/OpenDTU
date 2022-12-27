// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "ActivePowerControlCommand.h"
#include "inverters/InverterAbstract.h"

#define CRC_SIZE 6

ActivePowerControlCommand::ActivePowerControlCommand(uint64_t target_address, uint64_t router_address)
    : DevControlCommand(target_address, router_address)
{
    _payload[10] = 0x0b;
    _payload[11] = 0x00;
    _payload[12] = 0x00;
    _payload[13] = 0x00;
    _payload[14] = 0x00;
    _payload[15] = 0x00;

    udpateCRC(CRC_SIZE); // 2 byte crc

    _payload_size = 18;

    setTimeout(2000);
}

String ActivePowerControlCommand::getCommandName()
{
    return "ActivePowerControl";
}

void ActivePowerControlCommand::setActivePowerLimit(float limit, PowerLimitControlType type)
{
    uint16_t l = limit * 10;

    // limit
    _payload[12] = (l >> 8) & 0xff;
    _payload[13] = (l) & 0xff;

    // type
    _payload[14] = (type >> 8) & 0xff;
    _payload[15] = (type) & 0xff;

    udpateCRC(CRC_SIZE);
}

bool ActivePowerControlCommand::handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id)
{
    if (!DevControlCommand::handleResponse(inverter, fragment, max_fragment_id)) {
        return false;
    }

    if ((getType() == PowerLimitControlType::RelativNonPersistent) || (getType() == PowerLimitControlType::RelativPersistent)) {
        inverter->SystemConfigPara()->setLimitPercent(getLimit());
    } else {
        uint16_t max_power = inverter->DevInfo()->getMaxPower();
        if (max_power > 0) {
            inverter->SystemConfigPara()->setLimitPercent(static_cast<float>(getLimit()) / max_power * 100);
        } else {
            // TODO(tbnobody): Not implemented yet because we only can publish the percentage value
        }
    }
    inverter->SystemConfigPara()->setLastUpdateCommand(millis());
    inverter->SystemConfigPara()->setLastLimitCommandSuccess(CMD_OK);
    return true;
}

float ActivePowerControlCommand::getLimit()
{
    uint16_t l = (((uint16_t)_payload[12] << 8) | _payload[13]);
    return l / 10;
}

PowerLimitControlType ActivePowerControlCommand::getType()
{
    return (PowerLimitControlType)(((uint16_t)_payload[14] << 8) | _payload[15]);
}

void ActivePowerControlCommand::gotTimeout(InverterAbstract* inverter)
{
    inverter->SystemConfigPara()->setLastLimitCommandSuccess(CMD_NOK);
}