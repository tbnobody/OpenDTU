// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "InverterAbstract.h"

class HM_Abstract : public InverterAbstract {
public:
    explicit HM_Abstract(HoymilesRadio* radio, const uint64_t serial);
    bool sendStatsRequest();
    bool sendAlarmLogRequest(const bool force = false);
    bool sendDevInfoRequest();
    bool sendSystemConfigParaRequest();
    bool sendActivePowerControlRequest(float limit, const PowerLimitControlType type);
    bool resendActivePowerControlRequest();
    bool sendPowerControlRequest(const bool turnOn);
    bool sendRestartControlRequest();
    bool resendPowerControlRequest();
    bool sendGridOnProFileParaRequest();

private:
    uint8_t _lastAlarmLogCnt = 0;
    float _activePowerControlLimit = 0;
    PowerLimitControlType _activePowerControlType = PowerLimitControlType::AbsolutNonPersistent;

    uint8_t _powerState = 1;
};