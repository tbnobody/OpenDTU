#include "HM_Abstract.h"
#include "HoymilesRadio.h"
#include "commands/ActivePowerControlCommand.h"
#include "commands/AlarmDataCommand.h"
#include "commands/DevInfoAllCommand.h"
#include "commands/DevInfoSampleCommand.h"
#include "commands/RealTimeRunDataCommand.h"
#include "commands/SystemConfigParaCommand.h"

HM_Abstract::HM_Abstract(uint64_t serial)
    : InverterAbstract(serial) {};

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

bool HM_Abstract::sendAlarmLogRequest(HoymilesRadio* radio, bool force)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) {
        return false;
    }

    if (!force) {
        if (Statistics()->hasChannelFieldValue(CH0, FLD_EVT_LOG)) {
            if ((uint8_t)Statistics()->getChannelFieldValue(CH0, FLD_EVT_LOG) == _lastAlarmLogCnt) {
                return false;
            }
        }
    }

    _lastAlarmLogCnt = (uint8_t)Statistics()->getChannelFieldValue(CH0, FLD_EVT_LOG);

    time_t now;
    time(&now);

    AlarmDataCommand* cmd = radio->enqueCommand<AlarmDataCommand>();
    cmd->setTime(now);
    cmd->setTargetAddress(serial());
    EventLog()->setLastAlarmRequestSuccess(CMD_PENDING);

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
    SystemConfigPara()->setLastLimitRequestSuccess(CMD_PENDING);

    return true;
}

bool HM_Abstract::sendActivePowerControlRequest(HoymilesRadio* radio, float limit, PowerLimitControlType type)
{
    _activePowerControlLimit = limit;
    _activePowerControlType = type;

    ActivePowerControlCommand* cmd = radio->enqueCommand<ActivePowerControlCommand>();
    cmd->setActivePowerLimit(limit, type);
    cmd->setTargetAddress(serial());
    SystemConfigPara()->setLastLimitCommandSuccess(CMD_PENDING);

    return true;
}

bool HM_Abstract::resendActivePowerControlRequest(HoymilesRadio* radio)
{
    return sendActivePowerControlRequest(radio, _activePowerControlLimit, _activePowerControlType);
}