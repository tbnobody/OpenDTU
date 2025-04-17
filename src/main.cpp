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
    MessageOutput.printf("Starting OpenDTU\r\n");

    // Initialize file system
    MessageOutput.printf("Mounting FS...\r\n");
    if (!LittleFS.begin(false)) { // Do not format if mount failed
        MessageOutput.printf("Failed mounting FS... Trying to format...\r\n");
        const bool success = LittleFS.begin(true);
        MessageOutput.printf("FS reformat %s\r\n", success ? "successful" : "failed");
    }

    // Read configuration values
    MessageOutput.printf("Reading configuration...\r\n");
    Configuration.init(scheduler);
    if (!Configuration.read()) {
        bool success = Configuration.write();
        MessageOutput.printf("Failed to read configuration. New default configuration written %s\r\n",
            success ? "successful" : "failed");
    }
    if (Configuration.get().Cfg.Version != CONFIG_VERSION) {
        MessageOutput.printf("Performing configuration migration from %" PRIX32 " to %" PRIX32 "\r\n",
            Configuration.get().Cfg.Version, CONFIG_VERSION);
        Configuration.migrate();
    }

    // Read languate pack
    MessageOutput.printf("Reading language pack...\r\n");
    I18n.init(scheduler);

    // Load PinMapping
    MessageOutput.printf("Reading PinMapping...\r\n");
    if (PinMapping.init(Configuration.get().Dev_PinMapping)) {
        MessageOutput.printf("Found valid mapping\r\n");
    } else {
        MessageOutput.printf("Didn't found valid mapping. Using default.\r\n");
    }

    // Initialize Network
    MessageOutput.printf("Initializing Network...\r\n");
    NetworkSettings.init(scheduler);
    NetworkSettings.applyConfig();

    // Initialize NTP
    MessageOutput.printf("Initializing NTP...\r\n");
    NtpSettings.init();

    // Initialize SunPosition
    MessageOutput.printf("Initializing SunPosition...\r\n");
    SunPosition.init(scheduler);

    // Initialize MqTT
    MessageOutput.printf("Initializing MQTT...\r\n");
    MqttSettings.init();
    MqttHandleDtu.init(scheduler);
    MqttHandleInverter.init(scheduler);
    MqttHandleInverterTotal.init(scheduler);
    MqttHandleHass.init(scheduler);

    // Initialize WebApi
    MessageOutput.printf("Initializing WebApi...\r\n");
    WebApi.init(scheduler);

    // Initialize Display
    MessageOutput.printf("Initializing Display...\r\n");
    Display.init(scheduler);

    // Initialize Single LEDs
    MessageOutput.printf("Initializing LEDs...\r\n");
    LedSingle.init(scheduler);

    InverterSettings.init(scheduler);

    Datastore.init(scheduler);
    RestartHelper.init(scheduler);

    MessageOutput.printf("Startup complete\r\n");
}

void loop()
{
    scheduler.execute();
}
