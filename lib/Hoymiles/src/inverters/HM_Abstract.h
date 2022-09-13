#pragma once

#include "InverterAbstract.h"

class HM_Abstract : public InverterAbstract {
public:
    explicit HM_Abstract(uint64_t serial);
    bool sendStatsRequest(HoymilesRadio* radio);
    bool sendAlarmLogRequest(HoymilesRadio* radio);
    bool sendDevInfoRequest(HoymilesRadio* radio);
    bool sendSystemConfigParaRequest(HoymilesRadio* radio);
    bool sendActivePowerControlRequest(HoymilesRadio* radio, float limit, PowerLimitControlType type);

private:
    uint8_t _lastAlarmLogCnt = 0;
};