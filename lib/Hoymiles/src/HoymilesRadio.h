// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "TimeoutHelper.h"
#include "commands/CommandAbstract.h"
#include "types.h"
#include <memory>
#include <queue>

class HoymilesRadio {
public:
    serial_u DtuSerial();
    virtual void setDtuSerial(uint64_t serial);

    bool isIdle();
    bool isQueueEmpty();
    bool isInitialized();

    template <typename T>
    T* enqueCommand()
    {
        _commandQueue.push(std::make_shared<T>());
        return static_cast<T*>(_commandQueue.back().get());
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
    std::queue<std::shared_ptr<CommandAbstract>> _commandQueue;
    bool _isInitialized = false;
    bool _busyFlag = false;

    TimeoutHelper _rxTimeout;
};