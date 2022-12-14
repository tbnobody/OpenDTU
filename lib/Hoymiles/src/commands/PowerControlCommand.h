// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "DevControlCommand.h"

class PowerControlCommand : public DevControlCommand {
public:
    explicit PowerControlCommand(uint64_t target_address = 0, uint64_t router_address = 0);

    virtual String getCommandName();

    virtual bool handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id);
    virtual void gotTimeout(InverterAbstract* inverter);

    void setPowerOn(bool state);
    void setRestart();
};