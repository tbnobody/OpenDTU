// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Thomas Basler and others
 */

#include "CpuTemperature.h"
#include <Arduino.h>

#if defined(CONFIG_IDF_TARGET_ESP32)
// there is no official API available on the original ESP32
extern "C" {
uint8_t temprature_sens_read();
}
#elif defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3)
#include "driver/temp_sensor.h"
#endif

CpuTemperatureClass CpuTemperature;

float CpuTemperatureClass::read()
{
#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3)
    // Disabling temperature reading for ESP32-S2 models as it might lead to WDT resets.
    // Same issue with ESP32-S3, where such WDT resets have been observed multiple times.
    // See: https://github.com/espressif/esp-idf/issues/8088
    return NAN;
#endif

    std::lock_guard<std::mutex> lock(_mutex);

    float temperature = NAN;
    bool success = false;

#if defined(CONFIG_IDF_TARGET_ESP32)
    uint8_t raw = temprature_sens_read();
    ESP_LOGV(TAG, "Raw temperature value: %d", raw);
    temperature = (raw - 32) / 1.8f;
    success = (raw != 128);
#elif defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3)
    temp_sensor_config_t tsens = TSENS_CONFIG_DEFAULT();
    temp_sensor_set_config(tsens);
    temp_sensor_start();
#if defined(CONFIG_IDF_TARGET_ESP32S3) && (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 4, 3))
#error \
    "ESP32-S3 internal temperature sensor requires ESP IDF V4.4.3 or higher. See https://github.com/esphome/issues/issues/4271"
#endif
    esp_err_t result = temp_sensor_read_celsius(&temperature);
    temp_sensor_stop();
    success = (result == ESP_OK);
#endif

    if (success && std::isfinite(temperature)) {
        return temperature;
    } else {
        ESP_LOGD(TAG, "Ignoring invalid temperature (success=%d, value=%.1f)", success, temperature);
        return NAN;
    }
}
