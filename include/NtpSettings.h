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