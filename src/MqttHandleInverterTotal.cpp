// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "MqttHandleInverterTotal.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include <Hoymiles.h>

MqttHandleInverterTotalClass MqttHandleInverterTotal;

void MqttHandleInverterTotalClass::init()
{
    _lastPublish.set(Configuration.get().Mqtt_PublishInterval * 1000);
}

void MqttHandleInverterTotalClass::loop()
{
    if (!MqttSettings.getConnected() || !Hoymiles.isAllRadioIdle()) {
        return;
    }

    if (_lastPublish.occured()) {
        float totalAcPower = 0;
        float totalDcPower = 0;
        float totalDcPowerIrr = 0;
        float totalDcPowerIrrInst = 0;
        float totalAcYieldDay = 0;
        float totalAcYieldTotal = 0;
        uint8_t totalAcPowerDigits = 0;
        uint8_t totalDcPowerDigits = 0;
        uint8_t totalAcYieldDayDigits = 0;
        uint8_t totalAcYieldTotalDigits = 0;
        bool totalReachable = true;

        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);
            if (inv == nullptr || !inv->getEnablePolling()) {
                continue;
            }

            if (!inv->isReachable()) {
                totalReachable = false;
            }

            for (auto& c : inv->Statistics()->getChannelsByType(TYPE_AC)) {
                totalAcPower += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_PAC);
                totalAcPowerDigits = max<uint8_t>(totalAcPowerDigits, inv->Statistics()->getChannelFieldDigits(TYPE_AC, c, FLD_PAC));

                totalAcYieldDay += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_YD);
                totalAcYieldDayDigits = max<uint8_t>(totalAcYieldDayDigits, inv->Statistics()->getChannelFieldDigits(TYPE_AC, c, FLD_YD));

                totalAcYieldTotal += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_YT);
                totalAcYieldTotalDigits = max<uint8_t>(totalAcYieldTotalDigits, inv->Statistics()->getChannelFieldDigits(TYPE_AC, c, FLD_YT));
            }
            for (auto& c : inv->Statistics()->getChannelsByType(TYPE_DC)) {
                totalDcPower += inv->Statistics()->getChannelFieldValue(TYPE_DC, c, FLD_PDC);
                totalDcPowerDigits = max<uint8_t>(totalDcPowerDigits, inv->Statistics()->getChannelFieldDigits(TYPE_DC, c, FLD_PDC));

                if (inv->Statistics()->getStringMaxPower(c) > 0) {
                    totalDcPowerIrr += inv->Statistics()->getChannelFieldValue(TYPE_DC, c, FLD_PDC);
                    totalDcPowerIrrInst += inv->Statistics()->getStringMaxPower(c);
                }
            }
        }

        MqttSettings.publish("ac/power", String(totalAcPower, static_cast<unsigned int>(totalAcPowerDigits)));
        MqttSettings.publish("ac/yieldtotal", String(totalAcYieldTotal, static_cast<unsigned int>(totalAcYieldTotalDigits)));
        MqttSettings.publish("ac/yieldday", String(totalAcYieldDay, static_cast<unsigned int>(totalAcYieldDayDigits)));
        MqttSettings.publish("ac/is_valid", String(totalReachable));
        MqttSettings.publish("dc/power", String(totalDcPower, static_cast<unsigned int>(totalAcPowerDigits)));
        MqttSettings.publish("dc/irradiation", String(totalDcPowerIrrInst > 0 ? totalDcPowerIrr / totalDcPowerIrrInst * 100.0f : 0, 3));
        MqttSettings.publish("dc/is_valid", String(totalReachable));

        _lastPublish.set(Configuration.get().Mqtt_PublishInterval * 1000);
    }
}
