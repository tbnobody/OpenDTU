// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2025 Thomas Basler and others
 */
#include "Led_Single.h"
#include "Configuration.h"
#include "Datastore.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "PinMapping.h"
#include <Hoymiles.h>

LedSingleClass LedSingle;

/*
  The table is calculated using the following formula
  (See https://www.mikrocontroller.net/articles/LED-Fading)
  a = Step count: 101 --> 0 - 100
  b = PWM resolution: 256: 0 - 255
  y = Calculated value of index x:
      y = 0 if x = 0
      y = pow(2, log2(b-1) * (x+1) / a) if x > 0
*/
const uint8_t pwmTable[] = {
    0,
    1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 3, 3, 3, 3, 3,
    3, 4, 4, 4, 4, 4, 5, 5, 5, 5,
    6, 6, 6, 7, 7, 8, 8, 8, 9, 9,
    10, 11, 11, 12, 12, 13, 14, 15, 16, 16,
    17, 18, 19, 20, 22, 23, 24, 25, 27, 28,
    30, 32, 33, 35, 37, 39, 42, 44, 47, 49,
    52, 55, 58, 61, 65, 68, 72, 76, 81, 85,
    90, 95, 100, 106, 112, 118, 125, 132, 139, 147,
    156, 164, 174, 183, 194, 205, 216, 228, 241, 255
};

#define LED_OFF 0

LedSingleClass::LedSingleClass()
    : _setTask(LEDSINGLE_UPDATE_INTERVAL * TASK_MILLISECOND, TASK_FOREVER, std::bind(&LedSingleClass::setLoop, this))
    , _outputTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&LedSingleClass::outputLoop, this))
{
}

void LedSingleClass::init(Scheduler& scheduler)
{
    bool ledActive = false;

    _blinkTimeout.set(500);
    turnAllOn();

    const auto& pin = PinMapping.get();
    for (uint8_t i = 0; i < PINMAPPING_LED_COUNT; i++) {

        if (pin.led[i] > GPIO_NUM_NC) {
            pinMode(pin.led[i], OUTPUT);
            setLed(i, false);
            ledActive = true;
        }

        _ledMode[i] = LedState_t::Off;
    }

    if (ledActive) {
        scheduler.addTask(_outputTask);
        _outputTask.enable();

        scheduler.addTask(_setTask);
        _setTask.enable();
    }
}

void LedSingleClass::setLoop()
{
    if (_allMode == LedState_t::On) {
        const CONFIG_T& config = Configuration.get();

        // Update network status
        _ledMode[0] = LedState_t::Off;

        if (NetworkSettings.isConnected()) {
            _ledMode[0] = LedState_t::Blink;
        }

        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 5) && (!config.Mqtt.Enabled || (config.Mqtt.Enabled && MqttSettings.getConnected()))) {
            _ledMode[0] = LedState_t::On;
        }

        // Update inverter status
        _ledMode[1] = LedState_t::Off;
        if (Hoymiles.getNumInverters() && Datastore.getIsAtLeastOnePollEnabled()) {
            // set LED status
            if (Datastore.getIsAllEnabledReachable() && Datastore.getIsAllEnabledProducing()) {
                _ledMode[1] = LedState_t::On;
            }
            if (Datastore.getIsAllEnabledReachable() && !Datastore.getIsAllEnabledProducing()) {
                _ledMode[1] = LedState_t::Blink;
            }
        }

    } else if (_allMode == LedState_t::Off) {
        _ledMode[0] = LedState_t::Off;
        _ledMode[1] = LedState_t::Off;
    }
}

void LedSingleClass::outputLoop()
{
    for (uint8_t i = 0; i < PINMAPPING_LED_COUNT; i++) {
        switch (_ledMode[i]) {
        case LedState_t::Off:
            setLed(i, false);
            break;
        case LedState_t::On:
            setLed(i, true);
            break;
        case LedState_t::Blink:
            if (_blinkTimeout.occured()) {
                setLed(i, !_ledStateCurrent[i]);
                _blinkTimeout.reset();
            }
            break;
        }
    }
}

void LedSingleClass::setLed(const uint8_t ledNo, const bool ledState)
{
    const auto& pin = PinMapping.get();
    const auto& config = Configuration.get();

    if (pin.led[ledNo] == GPIO_NUM_NC) {
        return;
    }

    const uint32_t currentPWM = ledcRead(analogGetChannel(pin.led[ledNo]));
    const uint32_t targetPWM = ledState ? pwmTable[config.Led_Single[ledNo].Brightness] : LED_OFF;

    if (currentPWM == targetPWM) {
        return;
    }

    analogWrite(pin.led[ledNo], targetPWM);
    _ledStateCurrent[ledNo] = ledState;
}

void LedSingleClass::turnAllOff()
{
    _allMode = LedState_t::Off;
}

void LedSingleClass::turnAllOn()
{
    _allMode = LedState_t::On;
}
