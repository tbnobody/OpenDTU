// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 - 2023 Thomas Basler and others
 */
#include "NtpSettings.h"
#include "Configuration.h"
#include <Arduino.h>
#include <time.h>

NtpSettingsClass::NtpSettingsClass()
{
}

void NtpSettingsClass::init()
{
    setServer();
    setTimezone();
}

void NtpSettingsClass::setServer()
{
    configTime(0, 0, Configuration.get().Ntp.Server);
}

void NtpSettingsClass::setTimezone()
{
    setenv("TZ", Configuration.get().Ntp.Timezone, 1);
    tzset();
}

NtpSettingsClass NtpSettings;