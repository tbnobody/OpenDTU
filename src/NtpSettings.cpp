// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2026 Thomas Basler and others
 */
#include "NtpSettings.h"
#include "Configuration.h"
#include <MycilaNTP.h>

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
    Mycila::NTP.sync(Configuration.get().Ntp.Server);
}

void NtpSettingsClass::setTimezone()
{
    Mycila::NTP.setTimeZone(Configuration.get().Ntp.TimezoneDescr);
}

NtpSettingsClass NtpSettings;
