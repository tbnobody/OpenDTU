// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "MessageOutput.h"

#include <Arduino.h>

MessageOutputClass MessageOutput;

#define MSG_LOCK() \
    do {           \
    } while (xSemaphoreTake(_lock, portMAX_DELAY) != pdPASS)
#define MSG_UNLOCK() xSemaphoreGive(_lock)

MessageOutputClass::MessageOutputClass()
{
    _lock = xSemaphoreCreateMutex();
    MSG_UNLOCK();
}

void MessageOutputClass::register_ws_output(AsyncWebSocket* output)
{
    _ws = output;
}

size_t MessageOutputClass::write(uint8_t c)
{
    if (_buff_pos < BUFFER_SIZE) {
        MSG_LOCK();
        _buffer[_buff_pos] = c;
        _buff_pos++;
        MSG_UNLOCK();
    } else {
        _forceSend = true;
    }

    return Serial.write(c);
}

void MessageOutputClass::loop()
{
    // Send data via websocket if either time is over or buffer is full
    if (_forceSend || (millis() - _lastSend > 1000)) {
        MSG_LOCK();
        if (_ws && _buff_pos > 0) {
            _ws->textAll(_buffer, _buff_pos);
            _buff_pos = 0;
        }
        if (_forceSend) {
            _buff_pos = 0;
        }
        MSG_UNLOCK();
        _forceSend = false;
    }
}