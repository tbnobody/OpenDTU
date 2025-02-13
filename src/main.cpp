// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
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
    MessageOutput.println();
    MessageOutput.println("Starting OpenDTU");

    // Initialize file system
    MessageOutput.print("Initialize FS... ");
    if (!LittleFS.begin(false)) { // Do not format if mount failed
        MessageOutput.print("failed... trying to format...");
        if (!LittleFS.begin(true)) {
            MessageOutput.print("success");
        } else {
            MessageOutput.print("failed");
        }
    } else {
        MessageOutput.println("done");
    }

    // Read configuration values
    Configuration.init(scheduler);
    MessageOutput.print("Reading configuration... ");
    if (!Configuration.read()) {
        if (Configuration.write()) {
            MessageOutput.print("written... ");
        } else {
            MessageOutput.print("failed... ");
        }
    }
    if (Configuration.get().Cfg.Version != CONFIG_VERSION) {
        MessageOutput.print("migrated... ");
        Configuration.migrate();
    }
    MessageOutput.println("done");

    // Read languate pack
    MessageOutput.print("Reading language pack... ");
    I18n.init(scheduler);
    MessageOutput.println("done");

    // Load PinMapping
    MessageOutput.print("Reading PinMapping... ");
    if (PinMapping.init(Configuration.get().Dev_PinMapping)) {
        MessageOutput.print("found valid mapping ");
    } else {
        MessageOutput.print("using default config ");
    }
    MessageOutput.println("done");

    // Initialize Network
    MessageOutput.print("Initialize Network... ");
    NetworkSettings.init(scheduler);
    MessageOutput.println("done");
    NetworkSettings.applyConfig();

    // Initialize NTP
    MessageOutput.print("Initialize NTP... ");
    NtpSettings.init();
    MessageOutput.println("done");

    // Initialize SunPosition
    MessageOutput.print("Initialize SunPosition... ");
    SunPosition.init(scheduler);
    MessageOutput.println("done");

    // Initialize MqTT
    MessageOutput.print("Initialize MqTT... ");
    MqttSettings.init();
    MqttHandleDtu.init(scheduler);
    MqttHandleInverter.init(scheduler);
    MqttHandleInverterTotal.init(scheduler);
    MqttHandleHass.init(scheduler);
    MessageOutput.println("done");

    // Initialize WebApi
    MessageOutput.print("Initialize WebApi... ");
    WebApi.init(scheduler);
    MessageOutput.println("done");

    // Initialize Display
    MessageOutput.print("Initialize Display... ");
    Display.init(scheduler);
    MessageOutput.println("done");

    // Initialize Single LEDs
    MessageOutput.print("Initialize LEDs... ");
    LedSingle.init(scheduler);
    MessageOutput.println("done");

    InverterSettings.init(scheduler);

    Datastore.init(scheduler);
    RestartHelper.init(scheduler);
}

void loop()
{
    scheduler.execute();
}
