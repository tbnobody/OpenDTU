// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>

class BatteryClass {
public:
    uint32_t lastUpdate;

    float chargeVoltage;
    float chargeCurrentLimitation;
    float dischargeCurrentLimitation;
    uint16_t stateOfCharge;
    uint32_t stateOfChargeLastUpdate;
    uint16_t stateOfHealth;
    float voltage;
    float current;
    float temperature;
    bool alarmOverCurrentDischarge;
    bool alarmUnderTemperature;
    bool alarmOverTemperature;
    bool alarmUnderVoltage;
    bool alarmOverVoltage;

    bool alarmBmsInternal;
    bool alarmOverCurrentCharge;


    bool warningHighCurrentDischarge;
    bool warningLowTemperature;
    bool warningHighTemperature;
    bool warningLowVoltage;
    bool warningHighVoltage;

    bool warningBmsInternal;
    bool warningHighCurrentCharge;
    char manufacturer[9];
    bool chargeEnabled;
    bool dischargeEnabled;
    bool chargeImmediately;

private:
};

extern BatteryClass Battery;
