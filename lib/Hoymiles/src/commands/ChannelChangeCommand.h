// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "../HoymilesRadio_CMT.h"
#include "CommandAbstract.h"

class ChannelChangeCommand : public CommandAbstract {
public:
    explicit ChannelChangeCommand(InverterAbstract* inv, const uint64_t router_address = 0, const uint8_t channel = 0);

    virtual String getCommandName() const;

    void setChannel(const uint8_t channel);
    uint8_t getChannel() const;

    void setCountryMode(const CountryModeId_t mode);

    virtual bool handleResponse(const fragment_t fragment[], const uint8_t max_fragment_id);

    virtual uint8_t getMaxResendCount();
};
