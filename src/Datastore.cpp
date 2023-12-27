// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "Datastore.h"
#include "Configuration.h"
#include <Hoymiles.h>

DatastoreClass Datastore;

void DatastoreClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&DatastoreClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.setInterval(1 * TASK_SECOND);
    _loopTask.enable();
}

void DatastoreClass::loop()
{
    if (!Hoymiles.isAllRadioIdle()) {
        _loopTask.forceNextIteration();
        return;
    }

    uint8_t isProducing = 0;
    uint8_t isReachable = 0;
    uint8_t pollEnabledCount = 0;

    std::lock_guard<std::mutex> lock(_mutex);

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
}

float DatastoreClass::getTotalAcYieldTotalEnabled()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalAcYieldTotalEnabled;
}

float DatastoreClass::getTotalAcYieldDayEnabled()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalAcYieldDayEnabled;
}

float DatastoreClass::getTotalAcPowerEnabled()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalAcPowerEnabled;
}

float DatastoreClass::getTotalDcPowerEnabled()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalDcPowerEnabled;
}

float DatastoreClass::getTotalDcPowerIrradiation()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalDcPowerIrradiation;
}

float DatastoreClass::getTotalDcIrradiationInstalled()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalDcIrradiationInstalled;
}

float DatastoreClass::getTotalDcIrradiation()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalDcIrradiation;
}

uint32_t DatastoreClass::getTotalAcYieldTotalDigits()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalAcYieldTotalDigits;
}

uint32_t DatastoreClass::getTotalAcYieldDayDigits()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalAcYieldDayDigits;
}

uint32_t DatastoreClass::getTotalAcPowerDigits()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalAcPowerDigits;
}

uint32_t DatastoreClass::getTotalDcPowerDigits()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalDcPowerDigits;
}

bool DatastoreClass::getIsAtLeastOneReachable()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _isAtLeastOneReachable;
}

bool DatastoreClass::getIsAtLeastOneProducing()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _isAtLeastOneProducing;
}

bool DatastoreClass::getIsAllEnabledProducing()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _isAllEnabledProducing;
}

bool DatastoreClass::getIsAllEnabledReachable()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _isAllEnabledReachable;
}

bool DatastoreClass::getIsAtLeastOnePollEnabled()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _isAtLeastOnePollEnabled;
}
