// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "DevControlCommand.h"

typedef enum { // ToDo: to be verified by field tests
    AbsolutNonPersistent,
    RelativNonPersistent,
    AbsolutPersistent,
    RelativPersistent,
    PowerLimitControl_Max
} PowerLimitControlType;

class ActivePowerControlCommand : public DevControlCommand {
public:
    explicit ActivePowerControlCommand(InverterAbstract* inv, const uint64_t router_address = 0);

    virtual String getCommandName() const;
    virtual QueueInsertType getQueueInsertType() const { return QueueInsertType::RemoveOldest; }
    virtual bool areSameParameter(CommandAbstract* other);

    virtual bool handleResponse(const fragment_t fragment[], const uint8_t max_fragment_id);
    virtual void gotTimeout();

    void setActivePowerLimit(const float limit, const PowerLimitControlType type = RelativNonPersistent);
    float getLimit() const;
    PowerLimitControlType getType() const;
};
