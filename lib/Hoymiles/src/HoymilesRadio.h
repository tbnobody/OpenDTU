// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "TimeoutHelper.h"
#include "commands/CommandAbstract.h"
#include "types.h"
#include <memory>
#include <ThreadSafeQueue.h>

class HoymilesRadio {
public:
    serial_u DtuSerial() const;
    virtual void setDtuSerial(const uint64_t serial);

    bool isIdle() const;
    bool isQueueEmpty() const;
    bool isInitialized() const;

    void enqueCommand(std::shared_ptr<CommandAbstract> cmd)
    {
        _commandQueue.push(cmd);
    }

    template <typename T>
    std::shared_ptr<T> prepareCommand()
    {
        return std::make_shared<T>();
    }

protected:
    static serial_u convertSerialToRadioId(const serial_u serial);
    static void dumpBuf(const uint8_t buf[], const uint8_t len, const bool appendNewline = true);

    bool checkFragmentCrc(const fragment_t& fragment) const;
    virtual void sendEsbPacket(CommandAbstract& cmd) = 0;
    void sendRetransmitPacket(const uint8_t fragment_id);
    void sendLastPacketAgain();
    void handleReceivedPackage();

    serial_u _dtuSerial;
    ThreadSafeQueue<std::shared_ptr<CommandAbstract>> _commandQueue;
    bool _isInitialized = false;
    bool _busyFlag = false;

    TimeoutHelper _rxTimeout;
};