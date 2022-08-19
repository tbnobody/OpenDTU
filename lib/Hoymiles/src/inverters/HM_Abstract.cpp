#include "HM_Abstract.h"
#include "HoymilesRadio.h"
#include "commands/AlarmDataCommand.h"
#include "commands/DevInfoAllCommand.h"
#include "commands/DevInfoSampleCommand.h"
#include "commands/RealTimeRunDataCommand.h"
#include "commands/SystemConfigParaCommand.h"

HM_Abstract::HM_Abstract(uint64_t serial, Clock* clock)
    : InverterAbstract(serial, clock) {};

bool HM_Abstract::sendStatsRequest(HoymilesRadio* radio)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) {
        return false;
    }

    time_t now;
    time(&now);

    RealTimeRunDataCommand* cmd = radio->enqueCommand<RealTimeRunDataCommand>();
    cmd->setTime(now);
    cmd->setTargetAddress(serial());

    return true;
}

bool HM_Abstract::sendAlarmLogRequest(HoymilesRadio* radio)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) {
        return false;
    }

    if (Statistics()->hasChannelFieldValue(CH0, FLD_EVT_LOG)) {
        if ((uint8_t)Statistics()->getChannelFieldValue(CH0, FLD_EVT_LOG) == _lastAlarmLogCnt) {
            return false;
        }
    }

    _lastAlarmLogCnt = (uint8_t)Statistics()->getChannelFieldValue(CH0, FLD_EVT_LOG);

    time_t now;
    time(&now);

    AlarmDataCommand* cmd = radio->enqueCommand<AlarmDataCommand>();
    cmd->setTime(now);
    cmd->setTargetAddress(serial());

    return true;
}

bool HM_Abstract::sendDevInfoRequest(HoymilesRadio* radio)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) {
        return false;
    }

    time_t now;
    time(&now);

    DevInfoAllCommand* cmdAll = radio->enqueCommand<DevInfoAllCommand>();
    cmdAll->setTime(now);
    cmdAll->setTargetAddress(serial());

    DevInfoSampleCommand* cmdSample = radio->enqueCommand<DevInfoSampleCommand>();
    cmdSample->setTime(now);
    cmdSample->setTargetAddress(serial());

    return true;
}

bool HM_Abstract::sendSystemConfigParaRequest(HoymilesRadio* radio)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) {
        return false;
    }

    time_t now;
    time(&now);

    SystemConfigParaCommand* cmd = radio->enqueCommand<SystemConfigParaCommand>();
    cmd->setTime(now);
    cmd->setTargetAddress(serial());

    return true;
}