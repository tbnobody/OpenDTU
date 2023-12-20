// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <AsyncWebSocket.h>
#include <HardwareSerial.h>
#include <Stream.h>
#include <TaskSchedulerDeclarations.h>
#include <mutex>

#define BUFFER_SIZE 500

class MessageOutputClass : public Print {
public:
    void init(Scheduler& scheduler);
    size_t write(uint8_t c) override;
    size_t write(const uint8_t* buffer, size_t size) override;
    void register_ws_output(AsyncWebSocket* output);

private:
    void loop();

    Task _loopTask;

    AsyncWebSocket* _ws = nullptr;
    char _buffer[BUFFER_SIZE];
    uint16_t _buff_pos = 0;
    uint32_t _lastSend = 0;
    bool _forceSend = false;

    std::mutex _msgLock;
};

extern MessageOutputClass MessageOutput;