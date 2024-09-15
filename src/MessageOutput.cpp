// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "MessageOutput.h"
#include "SyslogLogger.h"

#include <Arduino.h>

MessageOutputClass MessageOutput;

MessageOutputClass::MessageOutputClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&MessageOutputClass::loop, this))
{
}

void MessageOutputClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.enable();
}

void MessageOutputClass::register_ws_output(AsyncWebSocket* output)
{
    _ws = output;
}

size_t MessageOutputClass::write(uint8_t c)
{
    Syslog.write(&c, 1);

    if (_buff_pos < BUFFER_SIZE) {
        std::lock_guard<std::mutex> lock(_msgLock);
        _buffer[_buff_pos] = c;
        _buff_pos++;
    } else {
        _forceSend = true;
    }

    return Serial.write(c);
}

size_t MessageOutputClass::write(const uint8_t* buffer, size_t size)
{
    Syslog.write(buffer, size);

    std::lock_guard<std::mutex> lock(_msgLock);
    if (_buff_pos + size < BUFFER_SIZE) {
        memcpy(&_buffer[_buff_pos], buffer, size);
        _buff_pos += size;
    }
    _forceSend = true;

    return Serial.write(buffer, size);
}

void MessageOutputClass::loop()
{
    // Send data via websocket if either time is over or buffer is full
    if (_forceSend || (millis() - _lastSend > 1000)) {
        std::lock_guard<std::mutex> lock(_msgLock);
        if (_ws && _buff_pos > 0) {
            _ws->textAll(_buffer, _buff_pos);
            _buff_pos = 0;
        }
        if (_forceSend) {
            _buff_pos = 0;
        }
        _forceSend = false;
    }
}
