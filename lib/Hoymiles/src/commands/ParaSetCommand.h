// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "CommandAbstract.h"

class ParaSetCommand : public CommandAbstract {
public:
    explicit ParaSetCommand(InverterAbstract* inv, const uint64_t router_address = 0);
};
