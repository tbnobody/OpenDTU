#include "NtpSettings.h"
#include "Configuration.h"
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
    configTime(0, 0, Configuration.get().Ntp_Server);
}

void NtpSettingsClass::setTimezone()
{
    setenv("TZ", Configuration.get().Ntp_Timezone, 1);
    tzset();
}

NtpSettingsClass NtpSettings;