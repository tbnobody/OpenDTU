// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include <HardwareSerial.h>
#include "MessageOutput.h"

MessageOutputClass MessageOutput;

void MessageOutputClass::register_ws_output(AsyncWebSocket* output)
{
    std::lock_guard<std::mutex> lock(_msgLock);

    _ws = output;
}

void MessageOutputClass::serialWrite(MessageOutputClass::message_t const& m)
{
    size_t written = 0;
    while (written < m.size()) {
        written += Serial.write(m.data() + written, m.size() - written);
    }
    Serial.flush();
}

size_t MessageOutputClass::write(uint8_t c)
{
    std::lock_guard<std::mutex> lock(_msgLock);

    auto res = _task_messages.emplace(xTaskGetCurrentTaskHandle(), message_t());
    auto iter = res.first;
    auto& message = iter->second;

    message.push_back(c);

    if (c == '\n') {
        serialWrite(message);
        _lines.emplace(std::move(message));
        _task_messages.erase(iter);
    }

    return 1;
}

size_t MessageOutputClass::write(const uint8_t *buffer, size_t size)
{
    std::lock_guard<std::mutex> lock(_msgLock);

    auto res = _task_messages.emplace(xTaskGetCurrentTaskHandle(), message_t());
    auto iter = res.first;
    auto& message = iter->second;

    message.reserve(message.size() + size);

    for (size_t idx = 0; idx < size; ++idx) {
        uint8_t c = buffer[idx];

        message.push_back(c);

        if (c == '\n') {
            serialWrite(message);
            _lines.emplace(std::move(message));
            message.clear();
            message.reserve(size - idx - 1);
        }
    }

    if (message.empty()) { _task_messages.erase(iter); }

    return size;
}

void MessageOutputClass::loop()
{
    std::lock_guard<std::mutex> lock(_msgLock);

    // clean up (possibly filled) buffers of deleted tasks
    auto map_iter = _task_messages.begin();
    while (map_iter != _task_messages.end()) {
        if (eTaskGetState(map_iter->first) == eDeleted) {
            map_iter = _task_messages.erase(map_iter);
            continue;
        }

        ++map_iter;
    }

    if (!_ws) {
        while (!_lines.empty()) {
            _lines.pop(); // do not hog memory
        }
        return;
    }

    while (!_lines.empty() && _ws->availableForWriteAll()) {
        _ws->textAll(std::make_shared<message_t>(std::move(_lines.front())));
        _lines.pop();
    }
}