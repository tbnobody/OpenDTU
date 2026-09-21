// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024-2026 Thomas Basler and others
 */
#include "CommandQueue.h"
#include "../inverters/InverterAbstract.h"
#include <algorithm>
#include <esp_log.h>

#undef TAG
static const char* TAG = "hoymiles";

void CommandQueue::removeAllEntriesForInverter(InverterAbstract* inv)
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = std::remove_if(_queue.begin(), _queue.end(),
        [&](const auto& v) { return v->getTargetAddress() == inv->serial(); });
    _queue.erase(it, _queue.end());
}

void CommandQueue::removeDuplicatedEntries(std::shared_ptr<CommandAbstract> cmd)
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = std::remove_if(_queue.begin() + 1, _queue.end(),
        [&](const auto& v) {
            return cmd->areSameParameter(v.get())
                && cmd.get()->getQueueInsertType() == QueueInsertType::RemoveOldest;
        });
    _queue.erase(it, _queue.end());
}

void CommandQueue::replaceEntries(std::shared_ptr<CommandAbstract> cmd)
{
    std::lock_guard<std::mutex> lock(_mutex);

    std::replace_if(_queue.begin() + 1, _queue.end(),
        [&](const auto& v) {
            return cmd.get()->getQueueInsertType() == QueueInsertType::ReplaceExistent
                && cmd->areSameParameter(v.get());
            },
        cmd
    );
}

uint8_t CommandQueue::countSimilarCommands(std::shared_ptr<CommandAbstract> cmd)
{
    std::lock_guard<std::mutex> lock(_mutex);

    return std::count_if(_queue.begin(), _queue.end(),
        [&](const auto& v) {
            return cmd->areSameParameter(v.get());
        });
}

void CommandQueue::dumpQueue() const
{
    // Only dump the queue when DEBUG logging is enabled for the hoymiles
    // module. This runs on every poll cycle for every inverter; dumping at
    // INFO level floods the log pipeline (MessageOutput buffer -> WebSocket
    // console -> async TCP stack) and starves the scheduler, freezing MQTT
    // and Live View updates.
    if (esp_log_level_get(TAG) < ESP_LOG_DEBUG) {
        return;
    }

    std::lock_guard<std::mutex> lock(_mutex);

    if (_queue.empty()) {
        return;
    }

    size_t pos = 0;
    for (const auto& entry : _queue) {
        ESP_LOGD(TAG, "%zu: %s", pos, entry->getCommandDescription().c_str());
        pos++;
    }
}
