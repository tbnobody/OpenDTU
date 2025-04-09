// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include <espMqttClient.h>
#include <string>
#include <vector>

typedef std::function<void(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len)> OnMessageCallback;

struct cb_filter_t {
    std::string topic;
    uint8_t qos;
    OnMessageCallback cb;
};

class MqttSubscribeParser {
public:
    void register_callback(const std::string& topic, uint8_t qos, const OnMessageCallback& cb);
    void unregister_callback(const std::string& topic);
    void handle_message(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len);
    std::vector<cb_filter_t> get_callbacks();

private:
    int mosquitto_topic_matches_sub(const char* sub, const char* topic, bool* result);

    std::vector<cb_filter_t> _callbacks;

    enum mosq_err_t {
        MOSQ_ERR_SUCCESS = 0,
        MOSQ_ERR_INVAL = 3,
    };
};
