#include "MqttAvailableHandler.h"
#include "MqttSettings.h"

MqttAvailableHandler::MqttAvailableHandler(MqttSendData sendDataFunction): _MqttSendData(sendDataFunction) {
    using std::placeholders::_1;
    MqttSettings.addOnPublishCallback(std::bind(&MqttAvailableHandler::onMqttPublished, this, _1));
}

void MqttAvailableHandler::send(const String &availableSubTopic, bool isAvailable) {
    if(isAvailable) {
        this->_unavailableMessageId = 0;
        this->_MqttSendData();
        MqttSettings.publish(availableSubTopic, String(isAvailable));
    } else {
        this->_unavailableMessageId = MqttSettings.publish(availableSubTopic, String(isAvailable), 1);
    }
}

void MqttAvailableHandler::onMqttPublished(uint16_t messageId) {
    if(this->_unavailableMessageId > 1 && messageId == this->_unavailableMessageId) {
        // unavailable message was successfully published

        // we can now publish our invalid data safely
        this->_MqttSendData();
    }
}