// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "Configuration.h"
#include "Display_Graphic.h"
#include "MessageOutput.h"
#include "MqttHandleDtu.h"
#include "MqttHandleHass.h"
#include "MqttHandleInverter.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "NtpSettings.h"
#include "PinMapping.h"
#include "Utils.h"
#include "WebApi.h"
#include "defaults.h"
#include <Arduino.h>
#include <Hoymiles.h>
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

    // Initialize inverter communication
    MessageOutput.print(F("Initialize Hoymiles interface... "));
    if (PinMapping.isValidNrf24Config()) {
        SPIClass* spiClass = new SPIClass(HSPI);
        spiClass->begin(pin.nrf24_clk, pin.nrf24_miso, pin.nrf24_mosi, pin.nrf24_cs);
        Hoymiles.setMessageOutput(&MessageOutput);
        Hoymiles.init(spiClass, pin.nrf24_en, pin.nrf24_irq);

        MessageOutput.println(F("  Setting radio PA level... "));
        Hoymiles.getRadio()->setPALevel((rf24_pa_dbm_e)config.Dtu_PaLevel);

        MessageOutput.println(F("  Setting DTU serial... "));
        Hoymiles.getRadio()->setDtuSerial(config.Dtu_Serial);

        MessageOutput.println(F("  Setting poll interval... "));
        Hoymiles.setPollInterval(config.Dtu_PollInterval);

        for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
            if (config.Inverter[i].Serial > 0) {
                MessageOutput.print(F("  Adding inverter: "));
                MessageOutput.print(config.Inverter[i].Serial, HEX);
                MessageOutput.print(F(" - "));
                MessageOutput.print(config.Inverter[i].Name);
                auto inv = Hoymiles.addInverter(
                    config.Inverter[i].Name,
                    config.Inverter[i].Serial);

                if (inv != nullptr) {
                    for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
                        inv->Statistics()->setStringMaxPower(c, config.Inverter[i].channel[c].MaxChannelPower);
                        inv->Statistics()->setChannelFieldOffset(TYPE_DC, static_cast<ChannelNum_t>(c), FLD_YT, config.Inverter[i].channel[c].YieldTotalOffset);
                    }
                }
                MessageOutput.println(F(" done"));
            }
        }
        MessageOutput.println(F("done"));
    } else {
        MessageOutput.println(F("Invalid pin config"));
    }
}

void loop()
{
    NetworkSettings.loop();
    yield();
    Hoymiles.loop();
    yield();
    MqttHandleDtu.loop();
    yield();
    MqttHandleInverter.loop();
    yield();
    MqttHandleHass.loop();
    yield();
    WebApi.loop();
    yield();
    Display.loop();
    yield();
    MessageOutput.loop();
    yield();
}