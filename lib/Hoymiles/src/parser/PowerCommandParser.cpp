// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "PowerCommandParser.h"

void PowerCommandParser::setLastPowerCommandSuccess(LastCommandSuccess status)
{
    _lastLimitCommandSuccess = status;
}

LastCommandSuccess PowerCommandParser::getLastPowerCommandSuccess()
{
    return _lastLimitCommandSuccess;
}

uint32_t PowerCommandParser::getLastUpdateCommand()
{
    return _lastUpdateCommand;
}

void PowerCommandParser::setLastUpdateCommand(uint32_t lastUpdate)
{
    _lastUpdateCommand = lastUpdate;
    setLastUpdate(lastUpdate);
}