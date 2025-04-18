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
    MessageOutput.printf("Starting OpenDTU\n");

    // Initialize file system
    MessageOutput.printf("Mounting FS...\n");
    if (!LittleFS.begin(false)) { // Do not format if mount failed
        MessageOutput.printf("Failed mounting FS... Trying to format...\n");
        const bool success = LittleFS.begin(true);
        MessageOutput.printf("FS reformat %s\n", success ? "successful" : "failed");
    }

    // Read configuration values
    MessageOutput.printf("Reading configuration...\n");
    Configuration.init(scheduler);
    if (!Configuration.read()) {
        bool success = Configuration.write();
        MessageOutput.printf("Failed to read configuration. New default configuration written %s\n",
            success ? "successful" : "failed");
    }
    if (Configuration.get().Cfg.Version != CONFIG_VERSION) {
        MessageOutput.printf("Performing configuration migration from %" PRIX32 " to %" PRIX32 "\n",
            Configuration.get().Cfg.Version, CONFIG_VERSION);
        Configuration.migrate();
    }

    // Read languate pack
    MessageOutput.printf("Reading language pack...\n");
    I18n.init(scheduler);

    // Load PinMapping
    MessageOutput.printf("Reading PinMapping...\n");
    if (PinMapping.init(Configuration.get().Dev_PinMapping)) {
        MessageOutput.printf("Found valid mapping\n");
    } else {
        MessageOutput.printf("Didn't found valid mapping. Using default.\n");
    }

    // Initialize Network
    MessageOutput.printf("Initializing Network...\n");
    NetworkSettings.init(scheduler);
    NetworkSettings.applyConfig();

    // Initialize NTP
    MessageOutput.printf("Initializing NTP...\n");
    NtpSettings.init();

    // Initialize SunPosition
    MessageOutput.printf("Initializing SunPosition...\n");
    SunPosition.init(scheduler);

    // Initialize MqTT
    MessageOutput.printf("Initializing MQTT...\n");
    MqttSettings.init();
    MqttHandleDtu.init(scheduler);
    MqttHandleInverter.init(scheduler);
    MqttHandleInverterTotal.init(scheduler);
    MqttHandleHass.init(scheduler);

    // Initialize WebApi
    MessageOutput.printf("Initializing WebApi...\n");
    WebApi.init(scheduler);

    // Initialize Display
    MessageOutput.printf("Initializing Display...\n");
    Display.init(scheduler);

    // Initialize Single LEDs
    MessageOutput.printf("Initializing LEDs...\n");
    LedSingle.init(scheduler);

    InverterSettings.init(scheduler);

    Datastore.init(scheduler);
    RestartHelper.init(scheduler);

    MessageOutput.printf("Startup complete\n");
}

void loop()
{
    scheduler.execute();
}
