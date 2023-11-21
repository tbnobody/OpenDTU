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

void LedSingleClass::init(Scheduler* scheduler)
{
    bool ledActive = false;

    _blinkTimeout.set(500);
    turnAllOn();

    auto& pin = PinMapping.get();
    for (uint8_t i = 0; i < PINMAPPING_LED_COUNT; i++) {

        if (pin.led[i] >= 0) {
            pinMode(pin.led[i], OUTPUT);
            digitalWrite(pin.led[i], LOW);
            ledActive = true;
        }

        _ledState[i] = LedState_t::Off;
    }

    if (ledActive) {
        scheduler->addTask(_outputTask);
        _outputTask.setCallback(std::bind(&LedSingleClass::outputLoop, this));
        _outputTask.setIterations(TASK_FOREVER);
        _outputTask.enable();

        scheduler->addTask(_setTask);
        _setTask.setCallback(std::bind(&LedSingleClass::setLoop, this));
        _setTask.setInterval(LEDSINGLE_UPDATE_INTERVAL * TASK_MILLISECOND);
        _setTask.setIterations(TASK_FOREVER);
        _setTask.enable();
    }
}

void LedSingleClass::setLoop()
{
    if (_allState == LedState_t::On) {
        const CONFIG_T& config = Configuration.get();

        // Update network status
        _ledState[0] = LedState_t::Off;

        if (NetworkSettings.isConnected()) {
            _ledState[0] = LedState_t::Blink;
        }

        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 5) && (!config.Mqtt.Enabled || (config.Mqtt.Enabled && MqttSettings.getConnected()))) {
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

    } else if (_allState == LedState_t::Off) {
        _ledState[0] = LedState_t::Off;
        _ledState[1] = LedState_t::Off;
    }
}

void LedSingleClass::outputLoop()
{
    auto& pin = PinMapping.get();
    for (uint8_t i = 0; i < PINMAPPING_LED_COUNT; i++) {

        if (pin.led[i] < 0) {
            continue;
        }

        switch (_ledState[i]) {
        case LedState_t::Off:
            digitalWrite(pin.led[i], LOW);
            break;
        case LedState_t::On:
            digitalWrite(pin.led[i], HIGH);
            break;
        case LedState_t::Blink:
            if (_blinkTimeout.occured()) {
                digitalWrite(pin.led[i], !digitalRead(pin.led[i]));
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
