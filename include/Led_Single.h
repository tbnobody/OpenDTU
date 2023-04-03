// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PinMapping.h"
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

private:
    enum class LedState_t {
        On,
        Off,
        Blink,
    };

    LedState_t _ledState[PINMAPPING_LED_COUNT];
    TimeoutHelper _updateTimeout;
    TimeoutHelper _blinkTimeout;
    uint8_t _ledActive = 0;
};

extern LedSingleClass LedSingle;