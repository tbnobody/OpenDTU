// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "Configuration.h"
#include "MqttHandleDtu.h"
#include "MqttHandleHass.h"
#include "MqttHandleInverter.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "NtpSettings.h"
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
    Serial.println();
    Serial.println(F("Starting OpenDTU"));

    // Initialize file system
    Serial.print(F("Initialize FS... "));
    if (!LittleFS.begin(false)) { // Do not format if mount failed
        Serial.print(F("failed... trying to format..."));
        if (!LittleFS.begin(true)) {
            Serial.print("success");
        } else {
            Serial.print("failed");
        }
    } else {
        Serial.println(F("done"));
    }

    // Read configuration values
    Serial.print(F("Reading configuration... "));
    if (!Configuration.read()) {
        Serial.print(F("initializing... "));
        Configuration.init();
        if (Configuration.write()) {
            Serial.print(F("written... "));
        } else {
            Serial.print(F("failed... "));
        }
    }
    if (Configuration.get().Cfg_Version != CONFIG_VERSION) {
        Serial.print(F("migrated... "));
        Configuration.migrate();
    }
    Serial.println(F("done"));

    // Initialize WiFi
    Serial.print(F("Initialize Network... "));
    NetworkSettings.init();
    Serial.println(F("done"));
    NetworkSettings.applyConfig();

    // Initialize NTP
    Serial.print(F("Initialize NTP... "));
    NtpSettings.init();
    Serial.println(F("done"));

    // Initialize MqTT
    Serial.print(F("Initialize MqTT... "));
    MqttSettings.init();
    MqttHandleDtu.init();
    MqttHandleInverter.init();
    MqttHandleHass.init();
    Serial.println(F("done"));

    // Initialize WebApi
    Serial.print(F("Initialize WebApi... "));
    WebApi.init();
    Serial.println(F("done"));

    // Check for default DTU serial
    Serial.print(F("Check for default DTU serial... "));
    CONFIG_T& config = Configuration.get();
    if (config.Dtu_Serial == DTU_SERIAL) {
        Serial.print(F("generate serial based on ESP chip id: "));
        uint64_t dtuId = Utils::generateDtuSerial();
        Serial.printf("%0x%08x... ",
            ((uint32_t)((dtuId >> 32) & 0xFFFFFFFF)),
            ((uint32_t)(dtuId & 0xFFFFFFFF)));
        config.Dtu_Serial = dtuId;
        Configuration.write();
    }
    Serial.println(F("done"));

    // Initialize inverter communication
    Serial.print(F("Initialize Hoymiles interface... "));
    SPIClass* spiClass = new SPIClass(HSPI);
    spiClass->begin(HOYMILES_PIN_SCLK, HOYMILES_PIN_MISO, HOYMILES_PIN_MOSI, HOYMILES_PIN_CS);
    Hoymiles.init(spiClass, HOYMILES_PIN_CE, HOYMILES_PIN_IRQ);

    Serial.println(F("  Setting radio PA level... "));
    Hoymiles.getRadio()->setPALevel((rf24_pa_dbm_e)config.Dtu_PaLevel);

    Serial.println(F("  Setting DTU serial... "));
    Hoymiles.getRadio()->setDtuSerial(config.Dtu_Serial);

    Serial.println(F("  Setting poll interval... "));
    Hoymiles.setPollInterval(config.Dtu_PollInterval);

    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        if (config.Inverter[i].Serial > 0) {
            Serial.print(F("  Adding inverter: "));
            Serial.print(config.Inverter[i].Serial, HEX);
            Serial.print(F(" - "));
            Serial.print(config.Inverter[i].Name);
            auto inv = Hoymiles.addInverter(
                config.Inverter[i].Name,
                config.Inverter[i].Serial);

            if (inv != nullptr) {
                for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
                    inv->Statistics()->setChannelMaxPower(c, config.Inverter[i].channel[c].MaxChannelPower);
                }
            }
            Serial.println(F(" done"));
        }
    }
    Serial.println(F("done"));
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
}