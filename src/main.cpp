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
    if (!LITTLEFS.begin()) {
        Serial.println(F("failed"));
    } else {
        Serial.println(F("done"));
    }
}

void loop()
{
    // put your main code here, to run repeatedly:
}