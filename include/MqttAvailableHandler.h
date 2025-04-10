#pragma once

#include "Arduino.h"
#include <functional>

typedef std::function<void()> MqttSendData;

class MqttAvailableHandler {
public:
    explicit MqttAvailableHandler(MqttSendData sendDataFunction);
    
    void send(const String &availableSubTopic, bool isAvailable);
    void onMqttPublished(uint16_t messageId);
private:
    MqttSendData _MqttSendData;

    uint16_t _unavailableMessageId = 0;
};