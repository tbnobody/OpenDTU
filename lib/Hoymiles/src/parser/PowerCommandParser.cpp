// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 - 2023 Thomas Basler and others
 */
#include "PowerCommandParser.h"

void PowerCommandParser::setLastPowerCommandSuccess(const LastCommandSuccess status)
{
    _lastLimitCommandSuccess = status;
}

LastCommandSuccess PowerCommandParser::getLastPowerCommandSuccess() const
{
    return _lastLimitCommandSuccess;
}

uint32_t PowerCommandParser::getLastUpdateCommand() const
{
    return _lastUpdateCommand;
}

void PowerCommandParser::setLastUpdateCommand(const uint32_t lastUpdate)
{
    _lastUpdateCommand = lastUpdate;
    setLastUpdate(lastUpdate);
}