// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 - 2023 Thomas Basler and others
 */
#include "Parser.h"

Parser::Parser()
{
    _xSemaphore = xSemaphoreCreateMutex();
    HOY_SEMAPHORE_GIVE(); // release before first use
}

uint32_t Parser::getLastUpdate() const
{
    return _lastUpdate;
}

void Parser::setLastUpdate(const uint32_t lastUpdate)
{
    _lastUpdate = lastUpdate;
}

void Parser::beginAppendFragment()
{
    HOY_SEMAPHORE_TAKE();
}

void Parser::endAppendFragment()
{
    HOY_SEMAPHORE_GIVE();
}