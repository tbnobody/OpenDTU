// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include "ZeroExportGridSource.h"
#include <espMqttClient.h>
#include <optional>

// Grid power source reading the grid power from an MQTT topic.
//
// The configured topic (ZeroExport.Mqtt.GridPowerTopic) must publish either
// a raw numeric value (e.g. "-123.4") or a JSON object containing a power
// field, mirroring the Shelly EM / 3EM status payloads:
//   {"id":0,"a_act_power":3.9,...,"total_act_power":103.61,...}
//   {"id":1,"current":2.681,"voltage":236.7,"act_power":-607.3,...}
//
// The subscription lives on OpenDTU's existing MQTT client, so no extra
// connection is created; the source is only usable while that client is
// connected (i.e. while the MQTT integration itself is enabled).
class ZeroExportMqttClass : public ZeroExportGridSource {
public:
    ZeroExportMqttClass();
    void start() override;
    void stop() override;
    bool isRunning() const override;
    void loop() override;

private:
    void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len);
    // Parse either a plain number or the configured field from a JSON object.
    static bool parsePower(const uint8_t* payload, size_t len, const char* dataType, float& outPower);

    bool _subscribed = false; // Whether the configured topic is subscribed.
    char _subscribedTopic[ZEROEXPORT_MAX_MQTT_TOPIC_STRLEN + 1] = { 0 };
};

extern ZeroExportMqttClass ZeroExportMqtt;