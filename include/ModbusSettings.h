// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

class ModbusSettingsClass {
public:
    ModbusSettingsClass();
    void init();

    void performConfig();

private:
    void startTCP();

    void stopTCP();
};

extern ModbusSettingsClass ModbusSettings;
