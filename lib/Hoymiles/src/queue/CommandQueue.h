// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "../commands/CommandAbstract.h"
#include <ThreadSafeQueue.h>
#include <memory>

class InverterAbstract;

class CommandQueue : public ThreadSafeQueue<std::shared_ptr<CommandAbstract>> {
public:
    void removeAllEntriesForInverter(InverterAbstract* inv);
    void removeDuplicatedEntries(std::shared_ptr<CommandAbstract> cmd);
    void replaceEntries(std::shared_ptr<CommandAbstract> cmd);

    uint8_t countSimilarCommands(std::shared_ptr<CommandAbstract> cmd);
};
