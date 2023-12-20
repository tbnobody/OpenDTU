// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PinMapping.h"
#include <TaskSchedulerDeclarations.h>
#include <TimeoutHelper.h>

#define LEDSINGLE_UPDATE_INTERVAL 2000

class LedSingleClass {
public:
    LedSingleClass();
    void init(Scheduler& scheduler);

    void turnAllOff();
    void turnAllOn();

private:
    void setLoop();
    void outputLoop();

    void setLed(const uint8_t ledNo, const bool ledState);

    Task _setTask;
    Task _outputTask;

    enum class LedState_t {
        On,
        Off,
        Blink,
    };

    LedState_t _ledMode[PINMAPPING_LED_COUNT];
    LedState_t _allMode;
    bool _ledStateCurrent[PINMAPPING_LED_COUNT];
    TimeoutHelper _blinkTimeout;
};

extern LedSingleClass LedSingle;