// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <WireGuard-ESP32.h>

class WireguardSettingsClass {
public:
    WireguardSettingsClass();
    void init();
    void loop();

private:
    WireGuard wg;
    void endWireGuard();
    void beginWireGuard();
};

extern WireguardSettingsClass WireguardSettings;