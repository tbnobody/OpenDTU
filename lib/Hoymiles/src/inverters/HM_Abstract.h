#pragma once

#include "InverterAbstract.h"

class HM_Abstract : public InverterAbstract {
public:
    HM_Abstract(uint64_t serial);
    bool sendStatsRequest(HoymilesRadio* radio) override;
    bool sendAlarmLogRequest(HoymilesRadio* radio) override;
    bool sendDevInfoRequest(HoymilesRadio* radio) override;

private:
    uint8_t _lastAlarmLogCnt = 0;
};