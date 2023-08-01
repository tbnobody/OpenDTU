// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PinMapping.h"
#include "defaults.h"
#include <TimeoutHelper.h>

#define LEDSINGLE_UPDATE_INTERVAL 2000

enum eLedFunction {
    CONNECTED_NETWORK,
    CONNECTED_MQTT,
    INV_REACHABLE,
    INV_PRODUCING,
};

class LedSingleClass {
public:
    LedSingleClass();
    void init();
    void loop();
    void setBrightness(uint8_t ledIndex, uint8_t brightness);
    uint8_t updateLED(PinMapping_t& pin, uint8_t ledIndex, uint8_t brightness);

    void turnAllOff();
    void turnAllOn();

private:
    enum class LedState_t {
        On,
        Off,
        Blink,
    };

    LedState_t _ledState[PINMAPPING_LED_COUNT];
    LedState_t _allState;
    TimeoutHelper _updateTimeout;
    TimeoutHelper _blinkTimeout;
    uint8_t _ledActive = 0;
    uint8_t _ledBrightnessSetting[PINMAPPING_LED_COUNT];
    uint8_t _ledCurrentBrightness[PINMAPPING_LED_COUNT];
};

extern LedSingleClass LedSingle;