// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <AsyncWebSocket.h>
#include <TaskSchedulerDeclarations.h>
#include <Print.h>
#include <freertos/task.h>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <queue>
#include <memory>

class MessageOutputClass : public Print {
public:
    MessageOutputClass();
    void init(Scheduler& scheduler);
    size_t write(uint8_t c) override;
    size_t write(const uint8_t* buffer, size_t size) override;
    void register_ws_output(AsyncWebSocket* output);

    static int log_vprintf(const char *fmt, va_list arguments);

private:
    void loop();

    Task _loopTask;

    using message_t = std::vector<uint8_t>;

    // we keep a buffer for every task and only write complete lines to the
    // serial output and then move them to be pushed through the websocket.
    // this way we prevent mangling of messages from different contexts.
    std::unordered_map<TaskHandle_t, message_t> _task_messages;
    std::queue<message_t> _lines;

    // we chunk the websocket output to circumvent issues with TCP delayed ACKs:
    // if the websocket client (Windows in particular) is using delayed ACKs,
    // and since we wait for an ACK before sending the next chunk, we will
    // accumulate way too many messages and we won't be able to send them out
    // fast enough as the rate of produced messages is higher than the rate of
    // ACKs received. by chunking and waiting in between chunks, we either
    // "motivate" the client to send out ACKs immediately as the TCP packets are
    // "large", or we will wait long enough for the TCP stack to send out the
    // ACK anyways.
    void send_ws_chunk(message_t&& line);
    static constexpr size_t WS_CHUNK_SIZE_BYTES = 512;
    static constexpr uint32_t WS_CHUNK_INTERVAL_MS = 250;
    std::shared_ptr<message_t> _ws_chunk = nullptr;
    uint32_t _last_ws_chunk_sent = 0;

    AsyncWebSocket* _ws = nullptr;

    std::mutex _msgLock;

    void serialWrite(message_t const& m);
};

extern MessageOutputClass MessageOutput;
