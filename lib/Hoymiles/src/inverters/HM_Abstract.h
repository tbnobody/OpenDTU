#pragma once

#include "InverterAbstract.h"

class HM_Abstract : public InverterAbstract {
public:
    HM_Abstract(uint64_t serial);
    bool sendStatsRequest(HoymilesRadio* radio);
    bool sendAlarmLogRequest(HoymilesRadio* radio);
    bool sendDevInfoRequest(HoymilesRadio* radio);

private:
    uint8_t _lastAlarmLogCnt = 0;
};