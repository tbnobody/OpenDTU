// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "ParaSetCommand.h"

ParaSetCommand::ParaSetCommand(InverterAbstract* inv, const uint64_t router_address)
    : CommandAbstract(inv, router_address)
{
    _payload[0] = 0x52;
}
