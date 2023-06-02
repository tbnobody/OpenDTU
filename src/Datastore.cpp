// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "Datastore.h"
#include "Configuration.h"
#include <Hoymiles.h>

DatastoreClass Datastore;

DatastoreClass::DatastoreClass()
{
}

void DatastoreClass::init()
{
    _updateTimeout.set(1000);
}

void DatastoreClass::loop()
{
    if (Hoymiles.isAllRadioIdle() && _updateTimeout.occured()) {

        uint8_t isProducing = 0;
        uint8_t isReachable = 0;

        totalAcYieldTotalEnabled = 0;
        totalAcYieldTotalDigits = 0;

        totalAcYieldDayEnabled = 0;
        totalAcYieldDayDigits = 0;

        totalAcPowerEnabled = 0;
        totalAcPowerDigits = 0;

        totalDcPowerEnabled = 0;
        totalDcPowerDigits = 0;

        totalDcPowerIrradiation = 0;
        totalDcIrradiationInstalled = 0;

        isAllEnabledProducing = true;
        isAllEnabledReachable = true;

        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);
            if (inv == nullptr) {
                continue;
            }

            auto cfg = Configuration.getInverterConfig(inv->serial());
            if (cfg == nullptr) {
                continue;
            }

            if (inv->isProducing()) {
                isProducing++;
            } else {
                if (inv->getEnablePolling()) {
                    isAllEnabledProducing = false;
                }
            }

            if (inv->isReachable()) {
                isReachable++;
            } else {
                if (inv->getEnablePolling()) {
                    isAllEnabledReachable = false;
                }
            }

            for (auto& c : inv->Statistics()->getChannelsByType(TYPE_AC)) {
                if (cfg->Poll_Enable) {
                    totalAcYieldTotalEnabled += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_YT);
                    totalAcYieldDayEnabled += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_YD);

                    totalAcYieldTotalDigits = max<unsigned int>(totalAcYieldTotalDigits, inv->Statistics()->getChannelFieldDigits(TYPE_AC, c, FLD_YT));
                    totalAcYieldDayDigits = max<unsigned int>(totalAcYieldDayDigits, inv->Statistics()->getChannelFieldDigits(TYPE_AC, c, FLD_YD));
                }
                if (inv->getEnablePolling()) {
                    totalAcPowerEnabled += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_PAC);
                    totalAcPowerDigits = max<unsigned int>(totalAcPowerDigits, inv->Statistics()->getChannelFieldDigits(TYPE_AC, c, FLD_PAC));
                }
            }

            for (auto& c : inv->Statistics()->getChannelsByType(TYPE_DC)) {
                if (inv->getEnablePolling()) {
                    totalDcPowerEnabled += inv->Statistics()->getChannelFieldValue(TYPE_DC, c, FLD_PDC);
                    totalDcPowerDigits = max<unsigned int>(totalDcPowerDigits, inv->Statistics()->getChannelFieldDigits(TYPE_DC, c, FLD_PDC));

                    if (inv->Statistics()->getStringMaxPower(c) > 0) {
                        totalDcPowerIrradiation += inv->Statistics()->getChannelFieldValue(TYPE_DC, c, FLD_PDC);
                        totalDcIrradiationInstalled += inv->Statistics()->getStringMaxPower(c);
                    }
                }
            }
        }

        isAtLeastOneProducing = isProducing > 0;
        isAtLeastOneReachable = isReachable > 0;

        totalDcIrradiation = totalDcIrradiationInstalled > 0 ? totalDcPowerIrradiation / totalDcIrradiationInstalled * 100.0f : 0;

        _updateTimeout.reset();
    }
}