// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Thomas Basler and others
 */
#include "CommandQueue.h"
#include "../inverters/InverterAbstract.h"
#include <algorithm>

void CommandQueue::removeAllEntriesForInverter(InverterAbstract* inv)
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = std::remove_if(_queue.begin(), _queue.end(),
        [&inv](std::shared_ptr<CommandAbstract> v) -> bool { return v.get()->getTargetAddress() == inv->serial(); });
    _queue.erase(it, _queue.end());
}

void CommandQueue::removeDuplicatedEntries(std::shared_ptr<CommandAbstract> cmd)
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = std::remove_if(_queue.begin() + 1, _queue.end(),
        [&cmd](std::shared_ptr<CommandAbstract> v) -> bool {
            return cmd->areSameParameter(v.get())
                && cmd.get()->getQueueInsertType() == QueueInsertType::RemoveOldest;
        });
    _queue.erase(it, _queue.end());
}

void CommandQueue::replaceEntries(std::shared_ptr<CommandAbstract> cmd)
{
    std::lock_guard<std::mutex> lock(_mutex);

    std::replace_if(_queue.begin() + 1, _queue.end(),
        [&cmd](std::shared_ptr<CommandAbstract> v)-> bool {
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
        [&cmd](std::shared_ptr<CommandAbstract> v) -> bool {
            return cmd->areSameParameter(v.get());
        });
}
