// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <TimeoutHelper.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class DatastoreClass {
public:
    DatastoreClass();
    void init();
    void loop();

    // Sum of yield total of all enabled inverters, a inverter which is just disabled at night is also included
    float getTotalAcYieldTotalEnabled();

    // Sum of yield day of all enabled inverters, a inverter which is just disabled at night is also included
    float getTotalAcYieldDayEnabled();

    // Sum of total AC power of all enabled inverters
    float getTotalAcPowerEnabled();

    // Sum of total DC power of all enabled inverters
    float getTotalDcPowerEnabled();

    // Sum of total DC power of all enabled inverters with maxStringPower set
    float getTotalDcPowerIrradiation();

    // Sum of total installed irradiation of all enabled inverters
    float getTotalDcIrradiationInstalled();

    // Percentage (1-100) of total irradiation
    float getTotalDcIrradiation();

    // Amount of relevant digits for yield total
    unsigned int getTotalAcYieldTotalDigits();

    // Amount of relevant digits for yield total
    unsigned int getTotalAcYieldDayDigits();

    // Amount of relevant digits for AC power
    unsigned int getTotalAcPowerDigits();

    // Amount of relevant digits for DC power
    unsigned int getTotalDcPowerDigits();

    // True, if at least one inverter is reachable
    bool getIsAtLeastOneReachable();

    // True if at least one inverter is producing
    bool getIsAtLeastOneProducing();

    // True if at least one inverter is enabled for polling
    bool getIsAtLeastOnePollEnabled();

    // True if all enabled inverters are producing
    bool getIsAllEnabledProducing();

    // True if all enabled inverters are reachable
    bool getIsAllEnabledReachable();

private:
    TimeoutHelper _updateTimeout;
    SemaphoreHandle_t _xSemaphore;

    float _totalAcYieldTotalEnabled = 0;
    float _totalAcYieldDayEnabled = 0;
    float _totalAcPowerEnabled = 0;
    float _totalDcPowerEnabled = 0;
    float _totalDcPowerIrradiation = 0;
    float _totalDcIrradiationInstalled = 0;
    float _totalDcIrradiation = 0;
    unsigned int _totalAcYieldTotalDigits = 0;
    unsigned int _totalAcYieldDayDigits = 0;
    unsigned int _totalAcPowerDigits = 0;
    unsigned int _totalDcPowerDigits = 0;
    bool _isAtLeastOneReachable = false;
    bool _isAtLeastOneProducing = false;
    bool _isAllEnabledProducing = false;
    bool _isAllEnabledReachable = false;
    bool _isAtLeastOnePollEnabled = false;
};

extern DatastoreClass Datastore;