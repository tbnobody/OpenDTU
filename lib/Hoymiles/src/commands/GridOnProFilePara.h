// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "MultiDataCommand.h"

class GridOnProFilePara : public MultiDataCommand {
public:
    explicit GridOnProFilePara(uint64_t target_address = 0, uint64_t router_address = 0, time_t time = 0);

    virtual String getCommandName();

    virtual bool handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id);
};