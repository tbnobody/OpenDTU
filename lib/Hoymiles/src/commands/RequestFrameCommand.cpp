// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "RequestFrameCommand.h"

RequestFrameCommand::RequestFrameCommand(uint64_t target_address, uint64_t router_address, uint8_t frame_no)
    : SingleDataCommand(target_address, router_address)
{
    if (frame_no > 127) {
        frame_no = 0;
    }
    setFrameNo(frame_no);
    _payload_size = 10;
}

String RequestFrameCommand::getCommandName()
{
    return "RequestFrame";
}

void RequestFrameCommand::setFrameNo(uint8_t frame_no)
{
    _payload[9] = frame_no | 0x80;
}

uint8_t RequestFrameCommand::getFrameNo()
{
    return _payload[9] & (~0x80);
}

bool RequestFrameCommand::handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id)
{
    return true;
}