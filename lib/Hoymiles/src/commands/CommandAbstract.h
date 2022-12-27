// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "types.h"
#include <Stream.h>
#include <cstdint>

#define RF_LEN 32

class InverterAbstract;

class CommandAbstract {
public:
    explicit CommandAbstract(uint64_t target_address = 0, uint64_t router_address = 0);
    virtual ~CommandAbstract() {};

    const uint8_t* getDataPayload();
    void dumpDataPayload(Print* stream);

    uint8_t getDataSize();

    void setTargetAddress(uint64_t address);
    uint64_t getTargetAddress();

    void setRouterAddress(uint64_t address);
    uint64_t getRouterAddress();

    void setTimeout(uint32_t timeout);
    uint32_t getTimeout();

    virtual String getCommandName() = 0;

    void setSendCount(uint8_t count);
    uint8_t getSendCount();
    uint8_t incrementSendCount();

    virtual CommandAbstract* getRequestFrameCommand(uint8_t frame_no);

    virtual bool handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id) = 0;
    virtual void gotTimeout(InverterAbstract* inverter);

protected:
    uint8_t _payload[RF_LEN];
    uint8_t _payload_size;
    uint32_t _timeout;
    uint8_t _sendCount;

    uint64_t _targetAddress;
    uint64_t _routerAddress;

private:
    void convertSerialToPacketId(uint8_t buffer[], uint64_t serial);
};