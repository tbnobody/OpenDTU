// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "Led_Single.h"
#include "Configuration.h"
#include "Datastore.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "PinMapping.h"
#include <Hoymiles.h>

LedSingleClass LedSingle;

LedSingleClass::LedSingleClass()
{
}

void LedSingleClass::init()
{
    _blinkTimeout.set(500);
    _updateTimeout.set(LEDSINGLE_UPDATE_INTERVAL);
    turnAllOn();

    for (uint8_t i = 0; i < PINMAPPING_LED_COUNT; i++) {
        auto& pin = PinMapping.get();

        if (pin.led[i] >= 0) {
            pinMode(pin.led[i], OUTPUT);
            setBrightness(i, LED_BRIGHTNESS);
            _ledCurrentBrightness[i] = 0;
            _ledActive++;
        }

        _ledState[i] = LedState_t::Off;
    }
}

void LedSingleClass::loop()
{
    if (_ledActive == 0) {
        return;
    }

    if (_updateTimeout.occured() && _allState == LedState_t::On) {
        const CONFIG_T& config = Configuration.get();

        // Update network status
        _ledState[0] = LedState_t::Off;

        if (NetworkSettings.isConnected()) {
            _ledState[0] = LedState_t::Blink;
        }

        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 5) && (!config.Mqtt_Enabled || (config.Mqtt_Enabled && MqttSettings.getConnected()))) {
            _ledState[0] = LedState_t::On;
        }

        // Update inverter status
        _ledState[1] = LedState_t::Off;
        if (Hoymiles.getNumInverters() && Datastore.getIsAtLeastOnePollEnabled()) {
            // set LED status
            if (Datastore.getIsAllEnabledReachable() && Datastore.getIsAllEnabledProducing()) {
                _ledState[1] = LedState_t::On;
            }
            if (Datastore.getIsAllEnabledReachable() && !Datastore.getIsAllEnabledProducing()) {
                _ledState[1] = LedState_t::Blink;
            }
        }

        _updateTimeout.reset();
    } else if (_updateTimeout.occured() && _allState == LedState_t::Off) {
        _ledState[0] = LedState_t::Off;
        _ledState[1] = LedState_t::Off;
    }

    for (uint8_t i = 0; i < PINMAPPING_LED_COUNT; i++) {
        auto& pin = PinMapping.get();

        if (pin.led[i] < 0) {
            continue;
        }

        switch (_ledState[i]) {
        case LedState_t::Off:
            _ledCurrentBrightness[i] = updateLED(pin, i, 0);
            break;
        case LedState_t::On:
            _ledCurrentBrightness[i] = updateLED(pin, i, _ledBrightnessSetting[i]);
            break;
        case LedState_t::Blink:
            if (_blinkTimeout.occured()) {
                _ledCurrentBrightness[i] = updateLED(pin, i, _ledBrightnessSetting[i] - _ledCurrentBrightness[i]);
                _blinkTimeout.reset();
            }
            break;
        }
    }
}

void LedSingleClass::turnAllOff()
{
    _allState = LedState_t::Off;
}

void LedSingleClass::turnAllOn()
{
    _allState = LedState_t::On;
}

void LedSingleClass::setBrightness(uint8_t ledIndex, uint8_t brightness)
{
    _ledBrightnessSetting[ledIndex] = brightness * 2.55f;
}

uint8_t LedSingleClass::updateLED(PinMapping_t& pin, uint8_t ledIndex, uint8_t newBrightness)
{
    if (newBrightness != _ledCurrentBrightness[ledIndex]) {
        newBrightness = (newBrightness > _ledBrightnessSetting[ledIndex]) ? _ledBrightnessSetting[ledIndex] : newBrightness; // ensure new brightness isn't over the brightness setting
        analogWrite(pin.led[ledIndex], newBrightness);
    }

    return newBrightness;
}
