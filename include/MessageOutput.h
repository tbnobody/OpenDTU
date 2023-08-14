// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <AsyncWebSocket.h>
#include <HardwareSerial.h>
#include <Stream.h>
#include <mutex>

#define BUFFER_SIZE 500

class MessageOutputClass : public Print {
public:
    void loop();
    size_t write(uint8_t c) override;
    size_t write(const uint8_t *buffer, size_t size) override;
    void register_ws_output(AsyncWebSocket* output);

private:
    AsyncWebSocket* _ws = NULL;
    char _buffer[BUFFER_SIZE];
    uint16_t _buff_pos = 0;
    uint32_t _lastSend = 0;
    bool _forceSend = false;

    std::mutex _msgLock;
};

extern MessageOutputClass MessageOutput;