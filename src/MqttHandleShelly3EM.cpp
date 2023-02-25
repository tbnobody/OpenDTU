// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Christoph Jans and others
 */
#include "MqttHandleShelly3EM.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include <Hoymiles.h>
#include "MessageOutput.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

MqttHandleShelly3EMClass MqttHandleShelly3EM;

void MqttHandleShelly3EMClass::init()
{
}

void MqttHandleShelly3EMClass::loop()
{
    if ( !MqttSettings.getConnected() ) {
        return;
    }

    const CONFIG_T& config = Configuration.get();

    if (millis() - _lastPublish > (config.Mqtt_PublishInterval * 1000)) {
        WiFiClient client;  // or WiFiClientSecure for HTTPS
        HTTPClient http;

        // Send request
        http.useHTTP10(true);
        http.begin(client, "http://192.168.1.103/status");
        int httpCode = http.GET();

        if (httpCode == 200) {
            // Parse response
            DynamicJsonDocument doc(2048);
            DeserializationError error = deserializeJson(doc, http.getStream());

            // Check for errors
            if (error) {
                MessageOutput.print(F("deserializeJson() failed: "));
                MessageOutput.println(error.c_str());
            } else{
                // Publish values
                MqttSettings.publishGeneric("shelly3EM/type", "SHEM-3", 0, Configuration.get().Mqtt_Retain); 
                MqttSettings.publishGeneric("shelly3EM/status/unixtime", doc["unixtime"], 0, Configuration.get().Mqtt_Retain);            
                MqttSettings.publishGeneric("shelly3EM/status/total_power", doc["total_power"], 0, Configuration.get().Mqtt_Retain);

                int i = 0;
                for (JsonObject emeter : doc["emeters"].as<JsonArray>()) {          
                    MqttSettings.publishGeneric("shelly3EM/status/"+String(i)+"/power", emeter["power"], 0, Configuration.get().Mqtt_Retain);
                    MqttSettings.publishGeneric("shelly3EM/status/"+String(i)+"/pf", emeter["pf"], 0, Configuration.get().Mqtt_Retain);
                    MqttSettings.publishGeneric("shelly3EM/status/"+String(i)+"/current", emeter["current"], 0, Configuration.get().Mqtt_Retain);
                    MqttSettings.publishGeneric("shelly3EM/status/"+String(i)+"/voltage", emeter["voltage"], 0, Configuration.get().Mqtt_Retain);
                    MqttSettings.publishGeneric("shelly3EM/status/"+String(i)+"/is_valid", emeter["is_valid"], 0, Configuration.get().Mqtt_Retain);
                    MqttSettings.publishGeneric("shelly3EM/status/"+String(i)+"/total", emeter["total"], 0, Configuration.get().Mqtt_Retain);
                    MqttSettings.publishGeneric("shelly3EM/status/"+String(i)+"/total_returned", emeter["total_returned"], 0, Configuration.get().Mqtt_Retain);
                    i++;
                }
            }
        }
        else{
            // Falls HTTP-Error
            MessageOutput.println("Shelly 3EM Connection Error: " +  String(httpCode));
        }
        _lastPublish = millis();

        // Disconnect
        http.end();
    }
}