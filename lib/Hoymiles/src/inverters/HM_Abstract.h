#pragma once

#include "InverterAbstract.h"
#include "Clock.h"

class HM_Abstract : public InverterAbstract {
public:
    explicit HM_Abstract(uint64_t serial, Clock* clock);
    bool sendStatsRequest(HoymilesRadio* radio);
    bool sendAlarmLogRequest(HoymilesRadio* radio);
    bool sendDevInfoRequest(HoymilesRadio* radio);
    bool sendSystemConfigParaRequest(HoymilesRadio* radio);

private:
    uint8_t _lastAlarmLogCnt = 0;
    Clock* _clock;
};