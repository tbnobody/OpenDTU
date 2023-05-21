// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <TimeoutHelper.h>

class DatastoreClass {
public:
    DatastoreClass();
    void init();
    void loop();

    // Sum of yield total of all enabled inverters, a inverter which is just disabled at night is also included
    float totalAcYieldTotalEnabled = 0;

    // Sum of yield day of all enabled inverters, a inverter which is just disabled at night is also included
    float totalAcYieldDayEnabled = 0;

    // Sum of total AC power of all enabled inverters
    float totalAcPowerEnabled = 0;

    // Sum of total DC power of all enabled inverters
    float totalDcPowerEnabled = 0;

    // Sum of total DC power of all enabled inverters with maxStringPower set
    float totalDcPowerIrradiation = 0;

    // Sum of total installed irradiation of all enabled inverters
    float totalDcIrradiationInstalled = 0;

    // Percentage (1-100) of total irradiation
    float totalDcIrradiation = 0;

    // Amount of relevant digits for yield total
    unsigned int totalAcYieldTotalDigits = 0;

    // Amount of relevant digits for yield total
    unsigned int totalAcYieldDayDigits = 0;

    // Amount of relevant digits for AC power
    unsigned int totalAcPowerDigits = 0;

    // Amount of relevant digits for DC power
    unsigned int totalDcPowerDigits = 0;

    // True, if at least one inverter is reachable
    bool isAtLeastOneReachable = false;

    // True if at least one inverter is producing
    bool isAtLeastOneProducing = false;

    // True if all enabled inverters are producing
    bool isAllEnabledProducing = false;

    // True if all enabled inverters are reachable
    bool isAllEnabledReachable = false;

private:
    TimeoutHelper _updateTimeout;
};

extern DatastoreClass Datastore;