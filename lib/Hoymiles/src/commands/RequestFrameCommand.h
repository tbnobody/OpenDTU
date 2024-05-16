// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "SingleDataCommand.h"

class RequestFrameCommand : public SingleDataCommand {
public:
    explicit RequestFrameCommand(InverterAbstract* inv, const uint64_t router_address = 0, uint8_t frame_no = 0);

    virtual String getCommandName() const;

    void setFrameNo(const uint8_t frame_no);
    uint8_t getFrameNo() const;

    virtual bool handleResponse(const fragment_t fragment[], const uint8_t max_fragment_id);
};
