// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "TimeoutHelper.h"
#include "commands/CommandAbstract.h"
#include "types.h"
#include <memory>
#include <ThreadSafeQueue.h>

class HoymilesRadio {
public:
    serial_u DtuSerial();
    virtual void setDtuSerial(uint64_t serial);

    bool isIdle();
    bool isQueueEmpty();
    bool isInitialized();

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
    static serial_u convertSerialToRadioId(serial_u serial);
    void dumpBuf(const uint8_t buf[], uint8_t len, bool appendNewline = true);

    bool checkFragmentCrc(fragment_t* fragment);
    virtual void sendEsbPacket(CommandAbstract* cmd) = 0;
    void sendRetransmitPacket(uint8_t fragment_id);
    void sendLastPacketAgain();
    void handleReceivedPackage();

    serial_u _dtuSerial;
    ThreadSafeQueue<std::shared_ptr<CommandAbstract>> _commandQueue;
    bool _isInitialized = false;
    bool _busyFlag = false;

    TimeoutHelper _rxTimeout;
};