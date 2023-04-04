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
        reason_str = "Vbat power on reset";
        break;
    case 3:
        reason_str = "Software reset digital core";
        break;
#if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32S2)
    case 4:
        reason_str = "Legacy watch dog reset digital core";
        break;
#endif
    case 5:
        reason_str = "Deep Sleep reset digital core";
        break;
#if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32S2)
    case 6:
        reason_str = "Reset by SLC module, reset digital core";
        break;
#endif
    case 7:
        reason_str = "Timer Group0 Watch dog reset digital core";
        break;
    case 8:
        reason_str = "Timer Group1 Watch dog reset digital core";
        break;
    case 9:
        reason_str = "RTC Watch dog Reset digital core";
        break;
    case 10:
        reason_str = "Instrusion tested to reset CPU";
        break;
    case 11:
        reason_str = "Time Group reset CPU";
        break;
    case 12:
        reason_str = "Software reset CPU";
        break;
    case 13:
        reason_str = "RTC Watch dog Reset CPU";
        break;
#if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32S2)
    case 14:
        reason_str = "for APP CPU, reset by PRO CPU";
        break;
#endif
    case 15:
        reason_str = "Reset when the vdd voltage is not stable";
        break;
    case 16:
        reason_str = "RTC Watch dog reset digital core and rtc module";
        break;
    default:
        reason_str = "NO_MEAN";
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
        reason_str = "POWERON_RESET";
        break;
    case 3:
        reason_str = "SW_RESET";
        break;
#if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32S2)
    case 4:
        reason_str = "OWDT_RESET";
        break;
#endif
    case 5:
        reason_str = "DEEPSLEEP_RESET";
        break;
#if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32S2)
    case 6:
        reason_str = "SDIO_RESET";
        break;
#endif
    case 7:
        reason_str = "TG0WDT_SYS_RESET";
        break;
    case 8:
        reason_str = "TG1WDT_SYS_RESET";
        break;
    case 9:
        reason_str = "RTCWDT_SYS_RESET";
        break;
    case 10:
        reason_str = "INTRUSION_RESET";
        break;
    case 11:
        reason_str = "TGWDT_CPU_RESET";
        break;
    case 12:
        reason_str = "SW_CPU_RESET";
        break;
    case 13:
        reason_str = "RTCWDT_CPU_RESET";
        break;
#if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32S2)
    case 14:
        reason_str = "EXT_CPU_RESET";
        break;
#endif
    case 15:
        reason_str = "RTCWDT_BROWN_OUT_RESET";
        break;
    case 16:
        reason_str = "RTCWDT_RTC_RESET";
        break;
    default:
        reason_str = "NO_MEAN";
    }

    return reason_str;
}

ResetReasonClass ResetReason;
