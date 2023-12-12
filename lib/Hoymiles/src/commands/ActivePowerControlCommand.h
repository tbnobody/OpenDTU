// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "DevControlCommand.h"

typedef enum { // ToDo: to be verified by field tests
    AbsolutNonPersistent = 0x0000, // 0
    RelativNonPersistent = 0x0001, // 1
    AbsolutPersistent = 0x0100, // 256
    RelativPersistent = 0x0101 // 257
} PowerLimitControlType;

class ActivePowerControlCommand : public DevControlCommand {
public:
    explicit ActivePowerControlCommand(const uint64_t target_address = 0, const uint64_t router_address = 0);

    virtual String getCommandName() const;

    virtual bool handleResponse(InverterAbstract& inverter, const fragment_t fragment[], const uint8_t max_fragment_id);
    virtual void gotTimeout(InverterAbstract& inverter);

    void setActivePowerLimit(const float limit, const PowerLimitControlType type = RelativNonPersistent);
    float getLimit() const;
    PowerLimitControlType getType();
};