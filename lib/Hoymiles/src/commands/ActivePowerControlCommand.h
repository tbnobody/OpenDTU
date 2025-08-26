// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "DevControlCommand.h"
#include <array>

typedef enum {
    AbsolutNonPersistent,
    RelativNonPersistent,
    AbsolutPersistent,
    RelativPersistent,
    PowerLimitControl_Max
} PowerLimitControlType;

typedef enum {
    HmActivePowerControl,
    HmsActivePowerControl,
} ActivePowerControlDeviceType;

typedef struct {
    ActivePowerControlDeviceType device;
    PowerLimitControlType controlType;
    uint32_t value;
} ActivePowerControlValue_t;

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

    void setDeviceType(ActivePowerControlDeviceType type);
    ActivePowerControlDeviceType getDeviceType() const;

private:
    static uint32_t getControlTypeValue(ActivePowerControlDeviceType deviceType, PowerLimitControlType limitType);

    ActivePowerControlDeviceType _deviceType = ActivePowerControlDeviceType::HmActivePowerControl;
    static const std::array<const ActivePowerControlValue_t, PowerLimitControlType::PowerLimitControl_Max * 2> _powerLimitControlTypeValues;
};
