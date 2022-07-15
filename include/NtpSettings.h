// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>

class NtpSettingsClass {
public:
    NtpSettingsClass();
    void init();

    void setServer();
    void setTimezone();
};

extern NtpSettingsClass NtpSettings;