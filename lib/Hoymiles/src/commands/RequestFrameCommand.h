#pragma once

#include "SingleDataCommand.h"

class RequestFrameCommand : public SingleDataCommand {
public:
    RequestFrameCommand(uint64_t target_address = 0, uint64_t router_address = 0, uint8_t frame_no = 0);

    void setFrameNo(uint8_t frame_no);
    uint8_t getFrameNo();

    virtual void handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id);
};