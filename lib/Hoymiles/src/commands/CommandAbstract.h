// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "types.h"
#include <Stream.h>
#include <cstdint>

#define RF_LEN 32
#define MAX_RESEND_COUNT 4 // Used if all packages are missing
#define MAX_RETRANSMIT_COUNT 5 // Used to send the retransmit package

class InverterAbstract;

class CommandAbstract {
public:
    explicit CommandAbstract(InverterAbstract* inv, const uint64_t router_address = 0);
    virtual ~CommandAbstract() {};

    const uint8_t* getDataPayload();
    void dumpDataPayload(Print* stream);

    uint8_t getDataSize() const;

    uint64_t getTargetAddress() const;

    void setRouterAddress(const uint64_t address);
    uint64_t getRouterAddress() const;

    void setTimeout(const uint32_t timeout);
    uint32_t getTimeout() const;

    virtual String getCommandName() const = 0;

    void setSendCount(const uint8_t count);
    uint8_t getSendCount() const;
    uint8_t incrementSendCount();

    virtual CommandAbstract* getRequestFrameCommand(const uint8_t frame_no);

    virtual bool handleResponse(const fragment_t fragment[], const uint8_t max_fragment_id) = 0;
    virtual void gotTimeout();

    // Sets the amount how often the specific command is resent if all fragments where missing
    virtual uint8_t getMaxResendCount() const;

    // Sets the amount how often a missing fragment is re-requested if it was not available
    virtual uint8_t getMaxRetransmitCount() const;

protected:
    uint8_t _payload[RF_LEN];
    uint8_t _payload_size;
    uint32_t _timeout;
    uint8_t _sendCount;

    uint64_t _targetAddress;
    uint64_t _routerAddress;

    InverterAbstract* _inv;

private:
    void setTargetAddress(const uint64_t address);
    static void convertSerialToPacketId(uint8_t buffer[], const uint64_t serial);
};
