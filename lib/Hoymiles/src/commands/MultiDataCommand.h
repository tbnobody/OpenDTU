// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "CommandAbstract.h"
#include "RequestFrameCommand.h"
#include <sys/time.h>

class MultiDataCommand : public CommandAbstract {
public:
    explicit MultiDataCommand(const uint64_t target_address = 0, const uint64_t router_address = 0, const uint8_t data_type = 0, const time_t time = 0);

    void setTime(const time_t time);
    time_t getTime() const;

    CommandAbstract* getRequestFrameCommand(const uint8_t frame_no);

    virtual bool handleResponse(InverterAbstract& inverter, const fragment_t fragment[], const uint8_t max_fragment_id);

protected:
    void setDataType(const uint8_t data_type);
    uint8_t getDataType() const;
    void udpateCRC();
    static uint8_t getTotalFragmentSize(const fragment_t fragment[], const uint8_t max_fragment_id);

    RequestFrameCommand _cmdRequestFrame;
};