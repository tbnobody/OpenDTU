#pragma once

#include "base/pluginmessages.h"

class MqttMessage : public PluginMessage {
public:
  MqttMessage(int senderId, int receiverId)
      : PluginMessage(TYPEIDS::MQTTMESSAGE_TYPE, senderId) {
    setReceiverId(receiverId);
  }
  ~MqttMessage() {

  }
  MqttMessage(const MqttMessage& m) = default;
  std::shared_ptr<char[]> topic;
  std::shared_ptr<uint8_t[]> payload;
  unsigned int length;
  bool appendTopic = true;
  void setMqtt(const char *ttopic, const uint8_t *data, size_t len) {
    length = len;
    topic = std::shared_ptr<char[]>(new char[strlen(ttopic)+1], std::default_delete<char[]>());
    strcpy(topic.get(), ttopic);
    // hack
    payload = std::shared_ptr<uint8_t[]>(new uint8_t[len] , std::default_delete<uint8_t[]>());
    memcpy(payload.get(), data, len);
  }

  std::shared_ptr<char[]> payloadToChar() {
    auto s = std::shared_ptr<char[]>(new char[length+1], std::default_delete<char[]>());
    s[length]='\0';
    memcpy(s.get(),payload.get(),length);
    return s;
  }
  
  int toString(char *buffer) {
    int c = sprintf(buffer, "MqttMessage{base=");
    c = c + PluginMessage::toString(buffer+c);
    c = c + sprintf(buffer+c," topic=%s, data=%s, append=%d}",topic.get(),payloadToChar().get(),appendTopic);
    return c;
  }
};

template <> struct EntityIds<MqttMessage> {
  enum { type_id = TYPEIDS::MQTTMESSAGE_TYPE };
};
