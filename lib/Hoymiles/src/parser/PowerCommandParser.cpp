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