// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "MultiDataCommand.h"

class AlarmDataCommand : public MultiDataCommand {
public:
    explicit AlarmDataCommand(InverterAbstract* inv, const uint64_t router_address = 0, const time_t time = 0);

    virtual String getCommandName() const;

    virtual bool handleResponse(const fragment_t fragment[], const uint8_t max_fragment_id);
    virtual void gotTimeout();
};
