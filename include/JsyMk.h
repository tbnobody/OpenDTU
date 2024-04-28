// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <JSY_MK.h>
#include <TaskSchedulerDeclarations.h>

class JsyMkClass {
public:
    enum class Field_t : size_t {
        // Device informations
        ADDRESS,
        MANUFACTURER,
        MODEL,
        VERSION,
        VOLTAGE_RANGE,
        CURRENT_RANGE,
        // Measures
        VOLTAGE,
        CURRENT,
        POWER,
        POWER_FACTOR,
        FREQUENCY,
        NEGATIVE,
        TOTAL_POSITIVE_ENERGY,
        TOTAL_NEGATIVE_ENERGY,
        TODAY_POSITIVE_ENERGY,
        TODAY_NEGATIVE_ENERGY
    };

    JsyMkClass();
    void init(Scheduler& scheduler);

    uint32_t getLastUpdate() const;
    bool isInitialised() const;

    uint32_t getPollInterval() const;
    void setPollInterval(const uint32_t interval);

    size_t getChannelNumber() const;

    String getFieldName(size_t channel, Field_t fieldId) const;
    String getFieldString(size_t channel, Field_t fieldId) const;
    float getFieldValue(size_t channel, Field_t fieldId) const;

    size_t getFieldDigits(Field_t fieldId) const;
    String getFieldUnit(Field_t fieldId) const;
    String getFieldDeviceClass(Field_t fieldId) const;
    String getFieldStatusClass(Field_t fieldId) const;

    void reset();

private:
    void loop();

    JSY_MK _jsymk;
    Task _loopTask;
    bool _initialised = false;
    uint32_t _lastUpdate = 0;
    float _todayPositiveRef = 0;
    float _todayNegativeRef = 0;
};

extern JsyMkClass JsyMk;