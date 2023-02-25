// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "Configuration.h"
#include "Display_Graphic.h"
#include "InverterSettings.h"
#include "MessageOutput.h"
#include "MqttHandleDtu.h"
#include "MqttHandleShelly3EM.h"
#include "MqttHandleHass.h"
#include "MqttHandleInverter.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "WireguardSettings.h"
#include "NtpSettings.h"
#include "PinMapping.h"
#include "SunPosition.h"
#include "Utils.h"
#include "WebApi.h"
#include "defaults.h"
#include <Arduino.h>
#include <LittleFS.h>

void setup()
{
    // Initialize serial output
    Serial.begin(SERIAL_BAUDRATE);
    while (!Serial)
        yield();
    MessageOutput.println();
    MessageOutput.println(F("Starting OpenDTU"));

    // Initialize file system
    MessageOutput.print(F("Initialize FS... "));
    if (!LittleFS.begin(false)) { // Do not format if mount failed
        MessageOutput.print(F("failed... trying to format..."));
        if (!LittleFS.begin(true)) {
            MessageOutput.print("success");
        } else {
            MessageOutput.print("failed");
        }
    } else {
        MessageOutput.println(F("done"));
    }

    // Read configuration values
    MessageOutput.print(F("Reading configuration... "));
    if (!Configuration.read()) {
        MessageOutput.print(F("initializing... "));
        Configuration.init();
        if (Configuration.write()) {
            MessageOutput.print(F("written... "));
        } else {
            MessageOutput.print(F("failed... "));
        }
    }
    if (Configuration.get().Cfg_Version != CONFIG_VERSION) {
        MessageOutput.print(F("migrated... "));
        Configuration.migrate();
    }
    CONFIG_T& config = Configuration.get();
    MessageOutput.println(F("done"));

    // Load PinMapping
    MessageOutput.print(F("Reading PinMapping... "));
    if (PinMapping.init(String(Configuration.get().Dev_PinMapping))) {
        MessageOutput.print(F("found valid mapping "));
    } else {
        MessageOutput.print(F("using default config "));
    }
    const PinMapping_t& pin = PinMapping.get();
    MessageOutput.println(F("done"));

    // Initialize WiFi
    MessageOutput.print(F("Initialize Network... "));
    NetworkSettings.init();
    MessageOutput.println(F("done"));
    NetworkSettings.applyConfig();

    // Initialize NTP
    MessageOutput.print(F("Initialize NTP... "));
    NtpSettings.init();
    MessageOutput.println(F("done"));

    // Initialize SunPosition
    MessageOutput.print(F("Initialize SunPosition... "));
    SunPosition.init();
    MessageOutput.println(F("done"));

    // Initialize MqTT
    MessageOutput.print(F("Initialize MqTT... "));
    MqttSettings.init();
    MqttHandleDtu.init();
    MqttHandleInverter.init();
    MqttHandleHass.init();
    MessageOutput.println(F("done"));

    // Initialize WebApi
    MessageOutput.print(F("Initialize WebApi... "));
    WebApi.init();
    MessageOutput.println(F("done"));

    // Initialize Display
    MessageOutput.print(F("Initialize Display... "));
    Display.init(
        static_cast<DisplayType_t>(pin.display_type),
        pin.display_data,
        pin.display_clk,
        pin.display_cs,
        pin.display_reset);
    Display.showLogo = config.Display_ShowLogo;
    Display.enablePowerSafe = config.Display_PowerSafe;
    Display.enableScreensaver = config.Display_ScreenSaver;
    Display.contrast = config.Display_Contrast;
    MessageOutput.println(F("done"));

    // Check for default DTU serial
    MessageOutput.print(F("Check for default DTU serial... "));
    if (config.Dtu_Serial == DTU_SERIAL) {
        MessageOutput.print(F("generate serial based on ESP chip id: "));
        uint64_t dtuId = Utils::generateDtuSerial();
        MessageOutput.printf("%0x%08x... ",
            ((uint32_t)((dtuId >> 32) & 0xFFFFFFFF)),
            ((uint32_t)(dtuId & 0xFFFFFFFF)));
        config.Dtu_Serial = dtuId;
        Configuration.write();
    }
    MessageOutput.println(F("done"));

    InverterSettings.init();

    MqttHandleShelly3EM.init();
    WireguardSettings.init();
}

void loop()
{
    NetworkSettings.loop();
    yield();
    WireguardSettings.loop();
    yield();
    InverterSettings.loop();
    yield();
    MqttHandleDtu.loop();
    yield();
    MqttHandleInverter.loop();
    yield();
    MqttHandleHass.loop();
    yield();
    MqttHandleShelly3EM.loop();
    yield();
    WebApi.loop();
    yield();
    Display.loop();
    yield();
    SunPosition.loop();
    yield();
    MessageOutput.loop();
    yield();
}