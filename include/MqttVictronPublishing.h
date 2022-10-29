// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <Arduino.h>
#include <Hoymiles.h>
#include <memory>

class MqttVictronPublishingClass {
public:
    void init();
    void loop();

private:
    void publishField(std::shared_ptr<InverterAbstract> inv, uint8_t invphase, uint8_t fieldId);

    uint32_t _lastPublishStats[INV_MAX_COUNT];
    uint32_t _lastPublish;

    uint8_t _publishFields[14] = {
        FLD_UDC,
        FLD_IDC,
        FLD_PDC,
        FLD_YD,
        FLD_YT,
        FLD_UAC,
        FLD_IAC,
        FLD_PAC,
        FLD_F,
        FLD_T,
        FLD_PCT,
        FLD_EFF,
        FLD_IRR,
        FLD_PRA
    };
};

extern MqttVictronPublishingClass MqttVictronPublishing;