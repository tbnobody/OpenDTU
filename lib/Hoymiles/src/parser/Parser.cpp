// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "Parser.h"

uint32_t Parser::getLastUpdate()
{
    return _lastUpdate;
}

void Parser::setLastUpdate(uint32_t lastUpdate)
{
    _lastUpdate = lastUpdate;
}