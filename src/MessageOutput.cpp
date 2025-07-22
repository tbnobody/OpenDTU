// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "MessageOutput.h"
#include "SyslogLogger.h"

MessageOutputClass MessageOutput;

MessageOutputClass::MessageOutputClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&MessageOutputClass::loop, this))
{
}

void MessageOutputClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.enable();
    memset(_buffer, 0, sizeof(_buffer));
    esp_log_set_vprintf(log_vprintf);
}

void MessageOutputClass::register_ws_output(AsyncWebSocket* output)
{
    std::lock_guard<std::mutex> lock(_msgLock);

    _ws = output;
}

int MessageOutputClass::log_vprintf(const char* fmt, va_list arguments)
{
    std::lock_guard<std::mutex> lock(MessageOutput._msgLock);
    return MessageOutput.log_vprintf_rate_limited(fmt, arguments);
}

bool MessageOutputClass::consumeToken()
{
    uint32_t now = millis();

    uint32_t elapsed = now - _last_token_refill_millis;
    size_t new_tokens = RATE_LIMIT_MAX_TOKENS * elapsed / RATE_LIMIT_WINDOW_MS;

    if (new_tokens > 0) {
        _available_tokens = std::min(_available_tokens + new_tokens, RATE_LIMIT_MAX_TOKENS);
        _last_token_refill_millis = now;
    }

    if (_available_tokens > 0) {
        --_available_tokens;
        return true;
    }

    return false;
}

int MessageOutputClass::log_self(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = log_vprintf_recursive(fmt, args);
    va_end(args);
    return ret;
}

int MessageOutputClass::log_vprintf_rate_limited(const char* fmt, va_list arguments)
{
    if (!consumeToken()) {
        if (_rate_limited_packets == 0) {
            _last_rate_limit_warning_millis = millis();
        }
        ++_rate_limited_packets;
        return 0;
    }

    if (_rate_limited_packets > 0) {
        uint32_t elapsed = millis() - _last_rate_limit_warning_millis;
        if (elapsed > RATE_LIMIT_WARNING_INTERVAL_MS) {
            log_self("W (%d) logging: Rate limited %d message%s in the last %d ms\n",
                millis(), _rate_limited_packets,
                (_rate_limited_packets > 1 ? "s" : ""), elapsed);
            _rate_limited_packets = 0;
            _last_rate_limit_warning_millis = millis();
        }
    }

    return log_vprintf_recursive(fmt, arguments);
}

int MessageOutputClass::log_vprintf_recursive(const char* fmt, va_list arguments)
{
    // we can only write up to the end of the buffer
    auto len = sizeof(_buffer) - _buffer_in;

    // do not overwrite messages that still need processing. be careful not to
    // overwrite the length prefix of the next message waiting to be processed,
    // i.e., the byte that _buffer_out points to.
    if (_buffer_out > _buffer_in) {
        len = _buffer_out - _buffer_in;
    }

    len -= 1; // leave room for length prefix

    auto prefix = &_buffer[_buffer_in];
    auto start = prefix + 1;
    auto written = vsnprintf(start, len, fmt, arguments);

    if (written < 0) {
        if (Serial) {
            static char const err_msg[] = "ERROR: vsnprintf failed: %d\n";
            char err_buf[sizeof(err_msg) + 10];
            auto err_written = snprintf(err_buf, sizeof(err_buf), err_msg, written);
            Serial.write(err_buf, err_written);
        }

        *prefix = 0;
        return written;
    }

    if (written >= len) {
        // buffer was too small, at least at the current location
        *prefix = 0;

        if (_buffer_in > 0 && _buffer_out <= _buffer_in) {
            // we can safely reset the buffer as there is no data ahead that
            // still needs processing, so we try again from the beginning.
            _buffer_in = 0;
            return log_vprintf_recursive(fmt, arguments);
        }

        if (Serial) {
            Serial.write("ERROR: log buffer overrun\n");
        }

        return -1;
    }

    written = std::min(written, 255); // effectively truncate to 255 bytes
    *prefix = static_cast<uint8_t>(written);
    _buffer_in += 1 + written; // now points to terminating null byte of message we just wrote
    return written;
}

void MessageOutputClass::serialWrite(const uint8_t* buffer, size_t size)
{
    // operator bool() of HWCDC returns false if the device is not attached to
    // a USB host. in general it makes sense to skip writing entirely if the
    // default serial port is not ready.
    if (!Serial) {
        return;
    }

    size_t written = 0;
    while (written < size) {
        written += Serial.write(buffer + written, size - written);
    }

    Serial.flush();
}

void MessageOutputClass::send_ws_chunk(const uint8_t* buffer, size_t size)
{
    if (!_ws) {
        return;
    }

    if (nullptr == _ws_chunk) {
        _ws_chunk = std::make_shared<message_t>();
        _ws_chunk->reserve(WS_CHUNK_SIZE_BYTES + TYPICAL_LINE_LENGTH); // add room for one more line
    }
    _ws_chunk->insert(_ws_chunk->end(), buffer, buffer + size);

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
    std::unique_lock<std::mutex> lock(_msgLock);

    while (_buffer_out != _buffer_in) {
        uint8_t msg_len = static_cast<uint8_t>(_buffer[_buffer_out]);
        if (msg_len == 0) {
            // we only read a null byte while we still have messages to process
            // if we are at the end of the buffer and the next message is at the
            // front. wrap around.
            _buffer_out = 0;
            continue;
        }

        lock.unlock();
        taskYIELD(); // allow high priority tasks to log while we process the buffer

        auto msg_start = reinterpret_cast<const uint8_t*>(&_buffer[_buffer_out]) + 1;
        serialWrite(msg_start, msg_len);
        Syslog.write(msg_start, msg_len);
        send_ws_chunk(msg_start, msg_len);

        lock.lock();

        _buffer_out += 1 + msg_len;
    }
}
