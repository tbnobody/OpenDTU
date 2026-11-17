// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "CommandAbstract.h"

class RfInfoCommand : public CommandAbstract {
public:
    explicit RfInfoCommand(InverterAbstract* inv, const uint64_t router_address = 0);

    String getCommandName() const override;

    bool handleResponse(const fragment_t fragment[], const uint8_t max_fragment_id) override;

    void setProbeFrequency(const uint32_t frequency);
    bool isProbe() const { return _probe; }

    uint8_t getMaxResendCount() const override;
    bool areSameParameter(CommandAbstract* other) override;

private:
    bool _probe = false;
};
