// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

class NtpSettingsClass {
public:
    NtpSettingsClass();
    void init();

    void setServer();
    void setTimezone();
};

extern NtpSettingsClass NtpSettings;