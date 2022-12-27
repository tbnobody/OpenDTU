// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Parser.h"
#include <Arduino.h>

class PowerCommandParser : public Parser {
public:
    void setLastPowerCommandSuccess(LastCommandSuccess status);
    LastCommandSuccess getLastPowerCommandSuccess();
    uint32_t getLastUpdateCommand();
    void setLastUpdateCommand(uint32_t lastUpdate);

private:
    LastCommandSuccess _lastLimitCommandSuccess = CMD_OK; // Set to OK because we have to assume nothing is done at startup

    uint32_t _lastUpdateCommand = 0;
};