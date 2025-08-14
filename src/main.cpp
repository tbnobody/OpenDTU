// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2025 Thomas Basler and others
 */
#include "Configuration.h"
#include "Datastore.h"
#include "Display_Graphic.h"
#include "I18n.h"
#include "InverterSettings.h"
#include "Led_Single.h"
#include "Logging.h"
#include "MessageOutput.h"
#include "MqttHandleDtu.h"
#include "MqttHandleHass.h"
#include "MqttHandleInverter.h"
#include "MqttHandleInverterTotal.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "NtpSettings.h"
#include "PinMapping.h"
#include "RestartHelper.h"
#include "Scheduler.h"
#include "SunPosition.h"
#include "Utils.h"
#include "WebApi.h"
#include "defaults.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <TaskScheduler.h>
#include <esp_heap_caps.h>

#undef TAG
static const char* TAG = "main";

void setup()
{
    // Move all dynamic allocations >512byte to psram (if available)
    heap_caps_malloc_extmem_enable(512);

    // Initialize serial output
    Serial.begin(SERIAL_BAUDRATE);
#if !ARDUINO_USB_CDC_ON_BOOT
    // Only wait for serial interface to be set up when not using CDC
    while (!Serial)
        yield();
#endif
    MessageOutput.init(scheduler);

    // For now, the log levels are just hard coded
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    esp_log_level_set("CORE", ESP_LOG_ERROR);

    ESP_LOGI(TAG, "Starting OpenDTU");

    // Initialize file system
    ESP_LOGI(TAG, "Mounting FS...");
    if (!LittleFS.begin(false)) { // Do not format if mount failed
        ESP_LOGW(TAG, "Failed mounting FS... Trying to format...");
        const bool success = LittleFS.begin(true);
        ESP_LOG_LEVEL_LOCAL((success ? ESP_LOG_INFO : ESP_LOG_ERROR), TAG, "FS reformat %s", success ? "successful" : "failed");
    }

    // Read configuration values
    ESP_LOGI(TAG, "Reading configuration...");
    Configuration.init(scheduler);
    if (!Configuration.read()) {
        bool success = Configuration.write();
        ESP_LOG_LEVEL_LOCAL((success ? ESP_LOG_INFO : ESP_LOG_WARN), TAG, "Failed to read configuration. New default configuration written %s",
            success ? "successful" : "failed");
    }
    if (Configuration.get().Cfg.Version != CONFIG_VERSION) {
        ESP_LOGI(TAG, "Performing configuration migration from %" PRIX32 " to %" PRIX32 "",
            Configuration.get().Cfg.Version, CONFIG_VERSION);
        Configuration.migrate();
    }

    // Set configured log levels
    Logging.applyLogLevels();
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);

    // Read languate pack
    ESP_LOGI(TAG, "Reading language pack...");
    I18n.init(scheduler);

    // Load PinMapping
    ESP_LOGI(TAG, "Reading PinMapping...");
    if (PinMapping.init(Configuration.get().Dev_PinMapping)) {
        ESP_LOGI(TAG, "Found valid mapping");
    } else {
        ESP_LOGW(TAG, "Didn't found valid mapping. Using default.");
    }

    // Initialize Network
    ESP_LOGI(TAG, "Initializing Network...");
    NetworkSettings.init(scheduler);
    NetworkSettings.applyConfig();

    // Initialize NTP
    ESP_LOGI(TAG, "Initializing NTP...");
    NtpSettings.init();

    // Initialize SunPosition
    ESP_LOGI(TAG, "Initializing SunPosition...");
    SunPosition.init(scheduler);

    // Initialize MqTT
    ESP_LOGI(TAG, "Initializing MQTT...");
    MqttSettings.init();
    MqttHandleDtu.init(scheduler);
    MqttHandleInverter.init(scheduler);
    MqttHandleInverterTotal.init(scheduler);
    MqttHandleHass.init(scheduler);

    // Initialize WebApi
    ESP_LOGI(TAG, "Initializing WebApi...");
    WebApi.init(scheduler);

    // Initialize Display
    ESP_LOGI(TAG, "Initializing Display...");
    Display.init(scheduler);

    // Initialize Single LEDs
    ESP_LOGI(TAG, "Initializing LEDs...");
    LedSingle.init(scheduler);

    InverterSettings.init(scheduler);

    Datastore.init(scheduler);
    RestartHelper.init(scheduler);

    ESP_LOGI(TAG, "Startup complete");
}

void loop()
{
    scheduler.execute();
}
