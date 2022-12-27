// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "InverterAbstract.h"

class HM_Abstract : public InverterAbstract {
public:
    explicit HM_Abstract(uint64_t serial);
    bool sendStatsRequest(HoymilesRadio* radio);
    bool sendAlarmLogRequest(HoymilesRadio* radio, bool force = false);
    bool sendDevInfoRequest(HoymilesRadio* radio);
    bool sendSystemConfigParaRequest(HoymilesRadio* radio);
    bool sendActivePowerControlRequest(HoymilesRadio* radio, float limit, PowerLimitControlType type);
    bool resendActivePowerControlRequest(HoymilesRadio* radio);
    bool sendPowerControlRequest(HoymilesRadio* radio, bool turnOn);
    bool sendRestartControlRequest(HoymilesRadio* radio);
    bool resendPowerControlRequest(HoymilesRadio* radio);

private:
    uint8_t _lastAlarmLogCnt = 0;
    float _activePowerControlLimit = 0;
    PowerLimitControlType _activePowerControlType = PowerLimitControlType::AbsolutNonPersistent;

    uint8_t _powerState = 1;
};