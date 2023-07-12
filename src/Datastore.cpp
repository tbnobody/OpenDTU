// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "Datastore.h"
#include "Configuration.h"
#include <Hoymiles.h>

#define DAT_SEMAPHORE_TAKE() \
    do {                     \
    } while (xSemaphoreTake(_xSemaphore, portMAX_DELAY) != pdPASS)
#define DAT_SEMAPHORE_GIVE() xSemaphoreGive(_xSemaphore)

DatastoreClass Datastore;

DatastoreClass::DatastoreClass()
{
    _xSemaphore = xSemaphoreCreateMutex();
    DAT_SEMAPHORE_GIVE(); // release before first use
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
        uint8_t pollEnabledCount = 0;

        DAT_SEMAPHORE_TAKE();

        _totalAcYieldTotalEnabled = 0;
        _totalAcYieldTotalDigits = 0;

        _totalAcYieldDayEnabled = 0;
        _totalAcYieldDayDigits = 0;

        _totalAcPowerEnabled = 0;
        _totalAcPowerDigits = 0;

        _totalDcPowerEnabled = 0;
        _totalDcPowerDigits = 0;

        _totalDcPowerIrradiation = 0;
        _totalDcIrradiationInstalled = 0;

        _isAllEnabledProducing = true;
        _isAllEnabledReachable = true;

        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);
            if (inv == nullptr) {
                continue;
            }

            auto cfg = Configuration.getInverterConfig(inv->serial());
            if (cfg == nullptr) {
                continue;
            }

            if (inv->getEnablePolling()) {
                pollEnabledCount++;
            }

            if (inv->isProducing()) {
                isProducing++;
            } else {
                if (inv->getEnablePolling()) {
                    _isAllEnabledProducing = false;
                }
            }

            if (inv->isReachable()) {
                isReachable++;
            } else {
                if (inv->getEnablePolling()) {
                    _isAllEnabledReachable = false;
                }
            }

            for (auto& c : inv->Statistics()->getChannelsByType(TYPE_AC)) {
                if (cfg->Poll_Enable) {
                    _totalAcYieldTotalEnabled += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_YT);
                    _totalAcYieldDayEnabled += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_YD);

                    _totalAcYieldTotalDigits = max<unsigned int>(_totalAcYieldTotalDigits, inv->Statistics()->getChannelFieldDigits(TYPE_AC, c, FLD_YT));
                    _totalAcYieldDayDigits = max<unsigned int>(_totalAcYieldDayDigits, inv->Statistics()->getChannelFieldDigits(TYPE_AC, c, FLD_YD));
                }
                if (inv->getEnablePolling()) {
                    _totalAcPowerEnabled += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_PAC);
                    _totalAcPowerDigits = max<unsigned int>(_totalAcPowerDigits, inv->Statistics()->getChannelFieldDigits(TYPE_AC, c, FLD_PAC));
                }
            }

            for (auto& c : inv->Statistics()->getChannelsByType(TYPE_DC)) {
                if (inv->getEnablePolling()) {
                    _totalDcPowerEnabled += inv->Statistics()->getChannelFieldValue(TYPE_DC, c, FLD_PDC);
                    _totalDcPowerDigits = max<unsigned int>(_totalDcPowerDigits, inv->Statistics()->getChannelFieldDigits(TYPE_DC, c, FLD_PDC));

                    if (inv->Statistics()->getStringMaxPower(c) > 0) {
                        _totalDcPowerIrradiation += inv->Statistics()->getChannelFieldValue(TYPE_DC, c, FLD_PDC);
                        _totalDcIrradiationInstalled += inv->Statistics()->getStringMaxPower(c);
                    }
                }
            }
        }

        _isAtLeastOneProducing = isProducing > 0;
        _isAtLeastOneReachable = isReachable > 0;
        _isAtLeastOnePollEnabled = pollEnabledCount > 0;

        _totalDcIrradiation = _totalDcIrradiationInstalled > 0 ? _totalDcPowerIrradiation / _totalDcIrradiationInstalled * 100.0f : 0;

        DAT_SEMAPHORE_GIVE();

        _updateTimeout.reset();
    }
}

float DatastoreClass::getTotalAcYieldTotalEnabled()
{
    DAT_SEMAPHORE_TAKE();
    float retval = _totalAcYieldTotalEnabled;
    DAT_SEMAPHORE_GIVE();
    return retval;
}

float DatastoreClass::getTotalAcYieldDayEnabled()
{
    DAT_SEMAPHORE_TAKE();
    float retval = _totalAcYieldDayEnabled;
    DAT_SEMAPHORE_GIVE();
    return retval;
}

float DatastoreClass::getTotalAcPowerEnabled()
{
    DAT_SEMAPHORE_TAKE();
    float retval = _totalAcPowerEnabled;
    DAT_SEMAPHORE_GIVE();
    return retval;
}

float DatastoreClass::getTotalDcPowerEnabled()
{
    DAT_SEMAPHORE_TAKE();
    float retval = _totalDcPowerEnabled;
    DAT_SEMAPHORE_GIVE();
    return retval;
}

float DatastoreClass::getTotalDcPowerIrradiation()
{
    DAT_SEMAPHORE_TAKE();
    float retval = _totalDcPowerIrradiation;
    DAT_SEMAPHORE_GIVE();
    return retval;
}

float DatastoreClass::getTotalDcIrradiationInstalled()
{
    DAT_SEMAPHORE_TAKE();
    float retval = _totalDcIrradiationInstalled;
    DAT_SEMAPHORE_GIVE();
    return retval;
}

float DatastoreClass::getTotalDcIrradiation()
{
    DAT_SEMAPHORE_TAKE();
    float retval = _totalDcIrradiation;
    DAT_SEMAPHORE_GIVE();
    return retval;
}

unsigned int DatastoreClass::getTotalAcYieldTotalDigits()
{
    DAT_SEMAPHORE_TAKE();
    unsigned int retval = _totalAcYieldTotalDigits;
    DAT_SEMAPHORE_GIVE();
    return retval;
}

unsigned int DatastoreClass::getTotalAcYieldDayDigits()
{
    DAT_SEMAPHORE_TAKE();
    unsigned int retval = _totalAcYieldDayDigits;
    DAT_SEMAPHORE_GIVE();
    return retval;
}

unsigned int DatastoreClass::getTotalAcPowerDigits()
{
    DAT_SEMAPHORE_TAKE();
    unsigned int retval = _totalAcPowerDigits;
    DAT_SEMAPHORE_GIVE();
    return retval;
}

unsigned int DatastoreClass::getTotalDcPowerDigits()
{
    DAT_SEMAPHORE_TAKE();
    unsigned int retval = _totalDcPowerDigits;
    DAT_SEMAPHORE_GIVE();
    return retval;
}

bool DatastoreClass::getIsAtLeastOneReachable()
{
    DAT_SEMAPHORE_TAKE();
    bool retval = _isAtLeastOneReachable;
    DAT_SEMAPHORE_GIVE();
    return retval;
}

bool DatastoreClass::getIsAtLeastOneProducing()
{
    DAT_SEMAPHORE_TAKE();
    bool retval = _isAtLeastOneProducing;
    DAT_SEMAPHORE_GIVE();
    return retval;
}

bool DatastoreClass::getIsAllEnabledProducing()
{
    DAT_SEMAPHORE_TAKE();
    bool retval = _isAllEnabledProducing;
    DAT_SEMAPHORE_GIVE();
    return retval;
}

bool DatastoreClass::getIsAllEnabledReachable()
{
    DAT_SEMAPHORE_TAKE();
    bool retval = _isAllEnabledReachable;
    DAT_SEMAPHORE_GIVE();
    return retval;
}

bool DatastoreClass::getIsAtLeastOnePollEnabled()
{
    DAT_SEMAPHORE_TAKE();
    bool retval = _isAtLeastOnePollEnabled;
    DAT_SEMAPHORE_GIVE();
    return retval;
}
