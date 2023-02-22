// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "Failsafe.h"
#include "Configuration.h"
// #include "MessageOutput.h"
// #include "PinMapping.h"
// #include "SunPosition.h"
#include "esp32-hal.h"
#include <Hoymiles.h>

FailsafeCheckClass FailsafeCheck;

void FailsafeCheckClass::init()
{
    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        _lastUpdate[i] = millis();
    }
}
void FailsafeCheckClass::RequestReceived(uint8_t pos)
{
    _lastUpdate[pos] = millis();
}

void FailsafeCheckClass::loop()
{
    const CONFIG_T& config = Configuration.get();
    float dcCurrent;

    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        auto const& inv_cfg = config.Inverter[i];
        if (inv_cfg.Serial == 0) {
            continue;
        }
        auto inv = Hoymiles.getInverterBySerial(inv_cfg.Serial);
        if (inv == nullptr) {
            continue;
        }

        if (inv_cfg.LimitFailsafeTimeInterval && ((_lastUpdate[i] + inv_cfg.LimitFailsafeTimeInterval) < millis())) {
            // Shut down inverter after Time set in LimitFailsafeTimeInterval
            Hoymiles.getMessageOutput()->printf("No temp. limit received for %f s. Inverter has been shut-down\r\n", (float)inv_cfg.LimitFailsafeTimeInterval / 1000);
            inv->sendPowerControlRequest(Hoymiles.getRadio(), 0);
            _lastUpdate[i] = millis(); // Prevents that we send shutdown too frequently
        }

        if (((_lastCurrentCheck + 10000) < millis())) {
            _lastCurrentCheck = millis();
            for (auto& t : inv->Statistics()->getChannelTypes()) {
                if (t == TYPE_DC) {
                    for (auto& c : inv->Statistics()->getChannelsByType(t)) {
                        dcCurrent = inv->Statistics()->getChannelFieldValue(t, c, FLD_IDC);
                        if (dcCurrent * 1000 > inv_cfg.MaxChannelCurrent) {
                            Hoymiles.getMessageOutput()->printf("Inverter shut-down\r\n");
                            inv->sendPowerControlRequest(Hoymiles.getRadio(), 0);
                            break;
                        }
                    }
                }
            }
        }
    }
}