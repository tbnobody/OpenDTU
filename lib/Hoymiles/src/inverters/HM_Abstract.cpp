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
    time_t now;
    if (!_clock->getNow(&now)) {
        return false;
    }

    RealTimeRunDataCommand* cmd = radio->enqueCommand<RealTimeRunDataCommand>();
    setCmdTimeAndSerial(cmd, &now);
    return true;
}

bool HM_Abstract::sendAlarmLogRequest(HoymilesRadio* radio)
{
    time_t now;
    if (!_clock->getNow(&now)) {
        return false;
    }

    if (Statistics()->hasChannelFieldValue(CH0, FLD_EVT_LOG)) {
        if ((uint8_t)Statistics()->getChannelFieldValue(CH0, FLD_EVT_LOG) == _lastAlarmLogCnt) {
            return false;
        }
    }

    _lastAlarmLogCnt = (uint8_t)Statistics()->getChannelFieldValue(CH0, FLD_EVT_LOG);

    AlarmDataCommand* cmd = radio->enqueCommand<AlarmDataCommand>();
    setCmdTimeAndSerial(cmd, &now);
    return true;
}

bool HM_Abstract::sendDevInfoRequest(HoymilesRadio* radio)
{
    time_t now;
    if (!_clock->getNow(&now)) {
        return false;
    }

    DevInfoAllCommand* cmdAll = radio->enqueCommand<DevInfoAllCommand>();
    setCmdTimeAndSerial(cmdAll, &now);

    DevInfoSampleCommand* cmdSample = radio->enqueCommand<DevInfoSampleCommand>();
    setCmdTimeAndSerial(cmdSample, &now);

    return true;
}

bool HM_Abstract::sendSystemConfigParaRequest(HoymilesRadio* radio)
{
    time_t now;
    if (!_clock->getNow(&now)) {
        return false;
    }

    SystemConfigParaCommand* cmd = radio->enqueCommand<SystemConfigParaCommand>();
    setCmdTimeAndSerial(cmd, &now);

    return true;
}

void HM_Abstract::setCmdTimeAndSerial(MultiDataCommand* cmd, time_t* now)
{
    cmd->setTime(*now);
    cmd->setTargetAddress(serial());
}