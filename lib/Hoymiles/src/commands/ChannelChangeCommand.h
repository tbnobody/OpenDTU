// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "CommandAbstract.h"

class ChannelChangeCommand : public CommandAbstract {
public:
    explicit ChannelChangeCommand(uint64_t target_address = 0, uint64_t router_address = 0, uint8_t channel = 0);

    virtual String getCommandName();

    void setChannel(uint8_t channel);
    uint8_t getChannel();

    virtual bool handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id);

    virtual uint8_t getMaxResendCount();
};