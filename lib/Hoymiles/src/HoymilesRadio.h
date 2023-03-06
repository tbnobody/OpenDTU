// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "commands/CommandAbstract.h"
#include "types.h"
#include <memory>
#include <queue>

class HoymilesRadio {
public:
    serial_u DtuSerial();
    virtual void setDtuSerial(uint64_t serial);

    template <typename T>
    T* enqueCommand()
    {
        _commandQueue.push(std::make_shared<T>());
        return static_cast<T*>(_commandQueue.back().get());
    }

protected:
    static serial_u convertSerialToRadioId(serial_u serial);
    void dumpBuf(const char* info, uint8_t buf[], uint8_t len);

    serial_u _dtuSerial;
    std::queue<std::shared_ptr<CommandAbstract>> _commandQueue;
};