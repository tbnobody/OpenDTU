// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "CommandAbstract.h"

class SingleDataCommand : public CommandAbstract {
public:
    explicit SingleDataCommand(uint64_t target_address = 0, uint64_t router_address = 0);
};