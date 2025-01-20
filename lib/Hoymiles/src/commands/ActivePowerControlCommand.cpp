// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */

/*
This command is used to send a limit to the inverter.

Derives from DevControlCommand.

Command structure:
SCmd: Sub-Command ID. Is always 0x0b
Limit: limit to be set in the inverter
Type: absolute / relative and persistant/non-persistant

00   01 02 03 04   05 06 07 08   09   10   11   12 13   14 15   16 17   18   19 20 21 22 23 24 25 26 27 28 29 30 31
-------------------------------------------------------------------------------------------------------------------
                                      |<------ CRC16 ------>|
51   71 60 35 46   80 12 23 04   81   0b   00   00 00   00 00   00 00   00   -- -- -- -- -- -- -- -- -- -- -- -- --
^^   ^^^^^^^^^^^   ^^^^^^^^^^^   ^^   ^^   ^^   ^^^^^   ^^^^^   ^^^^^   ^^
ID   Target Addr   Source Addr   Cmd  SCmd ?    Limit   Type    CRC16   CRC8
*/
#include "ActivePowerControlCommand.h"
#include "inverters/InverterAbstract.h"

#define CRC_SIZE 6

ActivePowerControlCommand::ActivePowerControlCommand(InverterAbstract* inv, const uint64_t router_address)
    : DevControlCommand(inv, router_address)
{
    _payload[10] = 0x0b;
    _payload[11] = 0x00;
    _payload[12] = 0x00;
    _payload[13] = 0x00;
    _payload[14] = 0x00;
    _payload[15] = 0x00;

    udpateCRC(CRC_SIZE); // 6 byte crc

    _payload_size = 18;

    setTimeout(2000);
}

String ActivePowerControlCommand::getCommandName() const
{
    char buffer[30];
    snprintf(buffer, sizeof(buffer), "ActivePowerControl (%02X)", getType());
    return buffer;
}

bool ActivePowerControlCommand::areSameParameter(CommandAbstract* other)
{
    return CommandAbstract::areSameParameter(other)
        && this->getType() == static_cast<ActivePowerControlCommand*>(other)->getType();
}

void ActivePowerControlCommand::setActivePowerLimit(const float limit, const PowerLimitControlType type)
{
    const uint16_t l = limit * 10;

    // limit
    _payload[12] = (l >> 8) & 0xff;
    _payload[13] = (l)&0xff;

    // type
    _payload[14] = (type >> 8) & 0xff;
    _payload[15] = (type)&0xff;

    udpateCRC(CRC_SIZE);
}

bool ActivePowerControlCommand::handleResponse(const fragment_t fragment[], const uint8_t max_fragment_id)
{
    if (!DevControlCommand::handleResponse(fragment, max_fragment_id)) {
        return false;
    }

    if ((getType() == PowerLimitControlType::RelativNonPersistent) || (getType() == PowerLimitControlType::RelativPersistent)) {
        _inv->SystemConfigPara()->setLimitPercent(getLimit());
    } else {
        const uint16_t max_power = _inv->DevInfo()->getMaxPower();
        if (max_power > 0) {
            _inv->SystemConfigPara()->setLimitPercent(static_cast<float>(getLimit()) / max_power * 100);
        } else {
            // TODO(tbnobody): Not implemented yet because we only can publish the percentage value
        }
    }
    _inv->SystemConfigPara()->setLastUpdateCommand(millis());
    std::shared_ptr<ActivePowerControlCommand> cmd(std::shared_ptr<ActivePowerControlCommand>(), this);
    if (_inv->getRadio()->countSimilarCommands(cmd) == 1) {
        _inv->SystemConfigPara()->setLastLimitCommandSuccess(CMD_OK);
    }
    return true;
}

float ActivePowerControlCommand::getLimit() const
{
    const float l = (static_cast<uint16_t>(_payload[12]) << 8) | _payload[13];
    return l / 10;
}

PowerLimitControlType ActivePowerControlCommand::getType() const
{
    return (PowerLimitControlType)((static_cast<uint16_t>(_payload[14]) << 8) | _payload[15]);
}

void ActivePowerControlCommand::gotTimeout()
{
    _inv->SystemConfigPara()->setLastLimitCommandSuccess(CMD_NOK);
}
