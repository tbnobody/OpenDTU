// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "ResetReason.h"

#if ESP_IDF_VERSION_MAJOR > 3 // IDF 4+
#if CONFIG_IDF_TARGET_ESP32 // ESP32/PICO-D4
#include <esp32/rom/rtc.h>
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/rom/rtc.h"
#else
#error Target CONFIG_IDF_TARGET is not supported
#endif
#else // ESP32 Before IDF 4.0
#include "rom/rtc.h"
#endif

String ResetReasonClass::get_reset_reason_verbose(uint8_t cpu_id)
{
    RESET_REASON reason;
    reason = rtc_get_reset_reason(cpu_id);

    String reason_str;

    switch (reason) {
    case 1:
        reason_str = F("Vbat power on reset");
        break;
    case 3:
        reason_str = F("Software reset digital core");
        break;
#ifndef CONFIG_IDF_TARGET_ESP32C3
    case 4:
        reason_str = F("Legacy watch dog reset digital core");
        break;
#endif
    case 5:
        reason_str = F("Deep Sleep reset digital core");
        break;
#ifndef CONFIG_IDF_TARGET_ESP32C3
    case 6:
        reason_str = F("Reset by SLC module, reset digital core");
        break;
#endif
    case 7:
        reason_str = F("Timer Group0 Watch dog reset digital core");
        break;
    case 8:
        reason_str = F("Timer Group1 Watch dog reset digital core");
        break;
    case 9:
        reason_str = F("RTC Watch dog Reset digital core");
        break;
    case 10:
        reason_str = F("Instrusion tested to reset CPU");
        break;
    case 11:
        reason_str = F("Time Group reset CPU");
        break;
    case 12:
        reason_str = F("Software reset CPU");
        break;
    case 13:
        reason_str = F("RTC Watch dog Reset CPU");
        break;
#ifndef CONFIG_IDF_TARGET_ESP32C3
    case 14:
        reason_str = F("for APP CPU, reseted by PRO CPU");
        break;
#endif
    case 15:
        reason_str = F("Reset when the vdd voltage is not stable");
        break;
    case 16:
        reason_str = F("RTC Watch dog reset digital core and rtc module");
        break;
    default:
        reason_str = F("NO_MEAN");
    }

    return reason_str;
}

String ResetReasonClass::get_reset_reason_short(uint8_t cpu_id)
{
    RESET_REASON reason;
    reason = rtc_get_reset_reason(cpu_id);

    String reason_str;

    switch (reason) {
    case 1:
        reason_str = F("POWERON_RESET");
        break;
    case 3:
        reason_str = F("SW_RESET");
        break;
#ifndef CONFIG_IDF_TARGET_ESP32C3
    case 4:
        reason_str = F("OWDT_RESET");
        break;
#endif
    case 5:
        reason_str = F("DEEPSLEEP_RESET");
        break;
#ifndef CONFIG_IDF_TARGET_ESP32C3
    case 6:
        reason_str = F("SDIO_RESET");
        break;
#endif
    case 7:
        reason_str = F("TG0WDT_SYS_RESET");
        break;
    case 8:
        reason_str = F("TG1WDT_SYS_RESET");
        break;
    case 9:
        reason_str = F("RTCWDT_SYS_RESET");
        break;
    case 10:
        reason_str = F("INTRUSION_RESET");
        break;
    case 11:
        reason_str = F("TGWDT_CPU_RESET");
        break;
    case 12:
        reason_str = F("SW_CPU_RESET");
        break;
    case 13:
        reason_str = F("RTCWDT_CPU_RESET");
        break;
#ifndef CONFIG_IDF_TARGET_ESP32C3
    case 14:
        reason_str = F("EXT_CPU_RESET");
        break;
#endif
    case 15:
        reason_str = F("RTCWDT_BROWN_OUT_RESET");
        break;
    case 16:
        reason_str = F("RTCWDT_RTC_RESET");
        break;
    default:
        reason_str = F("NO_MEAN");
    }

    return reason_str;
}

ResetReasonClass ResetReason;