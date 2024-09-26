// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "MessageOutput.h"
#include "SyslogLogger.h"
#include <HardwareSerial.h>

MessageOutputClass MessageOutput;

#undef TAG
#define TAG "MessageOutput"

MessageOutputClass::MessageOutputClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&MessageOutputClass::loop, this))
{
}

void MessageOutputClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.enable();
    esp_log_set_vprintf(log_vprintf);
}

void MessageOutputClass::register_ws_output(AsyncWebSocket* output)
{
    std::lock_guard<std::mutex> lock(_msgLock);

    _ws = output;
}

int MessageOutputClass::log_vprintf(const char* fmt, va_list arguments)
{
    char log_buffer[WS_CHUNK_SIZE_BYTES];
    vsnprintf(log_buffer, sizeof(log_buffer), fmt, arguments);
    return MessageOutput.print(log_buffer);
}

void MessageOutputClass::serialWrite(MessageOutputClass::message_t const& m)
{
    // operator bool() of HWCDC returns false if the device is not attached to
    // a USB host. in general it makes sense to skip writing entirely if the
    // default serial port is not ready.
    if (!Serial) {
        return;
    }

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

size_t MessageOutputClass::write(const uint8_t* buffer, size_t size)
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

    if (message.empty()) {
        _task_messages.erase(iter);
    }

    return size;
}

void MessageOutputClass::send_ws_chunk(message_t&& line)
{
    if (!_ws) {
        return;
    }

    if (nullptr == _ws_chunk) {
        _ws_chunk = std::make_shared<message_t>(std::move(line));
        _ws_chunk->reserve(WS_CHUNK_SIZE_BYTES + 128); // add room for one more line
    } else {
        _ws_chunk->insert(_ws_chunk->end(), line.begin(), line.end());
    }

    bool small = _ws_chunk->size() < WS_CHUNK_SIZE_BYTES;
    bool recent = (millis() - _last_ws_chunk_sent) < WS_CHUNK_INTERVAL_MS;
    if (small && recent) {
        return;
    }

    bool added_warning = false;
    for (auto& client : _ws->getClients()) {
        if (client.queueIsFull()) {
            continue;
        }

        client.text(_ws_chunk);

        // note that all clients will see the warning, even if only one
        // client is struggeling. however, this should be rare. we
        // won't be copying chunks around to avoid this. we do however,
        // avoid adding the warning multiple times.
        if (client.queueIsFull() && !added_warning) {
            static char const warningStr[] = "\nWARNING: websocket client's queue is full, expect log lines missing\n";
            _ws_chunk->insert(_ws_chunk->end(), warningStr, warningStr + sizeof(warningStr) - 1);
            added_warning = true;
        }
    }

    _ws_chunk = nullptr;
    _last_ws_chunk_sent = millis();
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

    while (!_lines.empty()) {
        Syslog.write(_lines.front().data(), _lines.front().size());
        send_ws_chunk(std::move(_lines.front()));
        _lines.pop();
    }
}
