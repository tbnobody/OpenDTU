// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <AsyncWebSocket.h>
#include <Print.h>
#include <freertos/task.h>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <queue>

class MessageOutputClass : public Print {
public:
    void loop();
    size_t write(uint8_t c) override;
    size_t write(const uint8_t *buffer, size_t size) override;
    void register_ws_output(AsyncWebSocket* output);

private:
    using message_t = std::vector<uint8_t>;

    // we keep a buffer for every task and only write complete lines to the
    // serial output and then move them to be pushed through the websocket.
    // this way we prevent mangling of messages from different contexts.
    std::unordered_map<TaskHandle_t, message_t> _task_messages;
    std::queue<message_t> _lines;

    AsyncWebSocket* _ws = nullptr;

    std::mutex _msgLock;
};

extern MessageOutputClass MessageOutput;