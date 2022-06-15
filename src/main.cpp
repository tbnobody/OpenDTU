#include "Configuration.h"
#include "Hoymiles.h"
#include "MqttPublishing.h"
#include "MqttSettings.h"
#include "NtpSettings.h"
#include "WebApi.h"
#include "WiFiSettings.h"
#include "defaults.h"
#include <Arduino.h>
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
    if (!LittleFS.begin()) {
        Serial.println(F("failed"));
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
    Serial.print(F("Initialize WiFi... "));
    WiFiSettings.init();
    Serial.println(F("done"));
    WiFiSettings.applyConfig();

    // Initialize NTP
    Serial.print(F("Initialize NTP... "));
    NtpSettings.init();
    Serial.println(F("done"));

    // Initialize MqTT
    Serial.print(F("Initialize MqTT... "));
    MqttSettings.init();
    MqttPublishing.init();
    Serial.println(F("done"));

    // Initialize WebApi
    Serial.print(F("Initialize WebApi... "));
    WebApi.init();
    Serial.println(F("done"));

    // Initialize inverter communication
    Serial.print(F("Initialize Hoymiles interface... "));
    CONFIG_T& config = Configuration.get();
    Hoymiles.init();
    Hoymiles.getRadio()->setPALevel((rf24_pa_dbm_e)config.Dtu_PaLevel);
    Hoymiles.getRadio()->setDtuSerial(config.Dtu_Serial);
    Hoymiles.setPollInterval(config.Dtu_PollInterval);

    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        if (config.Inverter[i].Serial > 0) {
            Hoymiles.addInverter(
                config.Inverter[i].Name,
                config.Inverter[i].Serial);
        }
    }
    Serial.println(F("done"));
}

void loop()
{
    WiFiSettings.loop();
    yield();
    Hoymiles.loop();
    yield();
    MqttPublishing.loop();
    yield();
}