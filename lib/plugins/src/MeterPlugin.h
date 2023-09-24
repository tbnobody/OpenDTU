#pragma once

#include "base/plugin.h"
#include "messages/metermessage.h"
#include "messages/mqttmessage.h"
#include <map>
#include <vector>

#ifndef MAX_NUM_METER
#define MAX_NUM_METER 5
#endif

typedef struct {
  String &getSerial() { return serial; }
  virtual void setSerial(String &_serial) { serial = _serial; }
  virtual bool hasSerial(String &s) { return serial.equals(s); }
  bool noSerial() { return serial.isEmpty(); }
  virtual float getPower() { return power; }
  void setPower(float p) {
    power = p;
    update = true;
  }
  virtual bool isUpdated() { return update; }
  void clearUpdated() { update = false; }

private:
  String serial;
  float power = 0;
  bool update = false;
} Meter;

template <std::size_t N> class meterarray : public structarray<Meter, N> {
public:
  meterarray() : structarray<Meter, N>() {}
  Meter *getMeterBySerial(String &serial) {
    return structarray<Meter, N>::getByKey(
        [&serial](Meter &m) { return m.hasSerial(serial); });
  }
  Meter *getEmptyIndex() {
    return structarray<Meter, N>::getByKey(
        [](Meter &s) { return s.noSerial(); });
  }
};

class MeterPlugin : public Plugin {
  enum pluginIds { METER_SERIAL, METER_POWER };

public:
  MeterPlugin() : Plugin(2, "meter") {}

  void setup() {}
  void loop() {
    for (int i = 0; i < meters.size(); i++) {
      if (meters[i].isUpdated()) {
        meters[i].clearUpdated();
        publishAC(meters[i]);
      }
    }
  }
  Meter *getIndex(String &serial) {
    Meter *ms = meters.getMeterBySerial(serial);
    if (ms == nullptr) {
      ms = meters.getEmptyIndex();
      if (ms != nullptr) {
        ms->setSerial(serial);
      }
    }
    return ms;
  }
  void setMeterConsumption(String &serial, float consumption) {
    PDebug.printf(PDebugLevel::DEBUG,"meterplugin: setMeterConsumption(%s,%f)\n",
                         serial.c_str(), consumption);
    Meter *ms = getIndex(serial);
    if (ms == nullptr) {
      PDebug.printf(PDebugLevel::DEBUG,"meterplugin: meter[%s] not found!", serial.c_str());
      return;
    }
    ms->setPower(consumption);
  }
  void mqttCallback( MqttMessage *message) {
    PDebug.printf(PDebugLevel::DEBUG,"meterplugin: mqttCallback %s = %s\n", message->topic.get(), message->payloadToChar().get());
    char buffer[message->length + 1];
    buffer[message->length] = '\0';
    mempcpy(buffer, message->payload.get(), message->length);
    if (topicMap.find(message->topic.get()) != topicMap.end()) {
      String serial = topicMap[message->topic.get()];
      float cons = atof(buffer);
      setMeterConsumption(serial, cons);
    } else {
      DynamicJsonDocument doc(sizeof(buffer));
      DeserializationError error = deserializeJson(doc, buffer);
      if (error) {
        PDebug.printf(PDebugLevel::DEBUG,"meterplugin: Failed to deserialize %s\n", buffer);
      } else {
        JsonObject o = doc.as<JsonObject>();
        if (!onRequest(o, o)) {
          PDebug.printf(PDebugLevel::DEBUG,"meterplugin: onRequest returned error\n");
        }
      }
    }
  }

  bool onRequest(JsonObject request, JsonObject response) {
    // {"plugin":"meter","meterserial":2394858458,"power":280}
    if (request.containsKey("meterserial") && request.containsKey("power")) {
      String serial = request["meterserial"].as<String>();
      float consumption = request["power"];
      setMeterConsumption(serial, consumption);
      return true;
    } else {
      return false;
    }
  }

  void publishAC(Meter &meter) {
    PDebug.printf(PDebugLevel::DEBUG,"meterplugin: publishPower[%s]: %f W \n",
                         meter.getSerial().c_str(), meter.getPower());
    MeterMessage m(*this);
    m.power = meter.getPower();
    m.serial = meter.getSerial();
    publishMessage(m);
  }
  void saveSettings(JsonObject settings) {
    settings[F("meter_mqtt_topics")] = meter_mqtt_topics;
    settings[F("meter_mqtt_json_topics")] = meter_mqtt_json_topics;
  }
  void loadSettings(JsonObject settings) {
    if (settings.containsKey(F("meter_mqtt_json_topics"))) {
      meter_mqtt_json_topics =
          settings[F("meter_mqtt_json_topics")].as<String>();
      initTopics(meter_mqtt_json_topics);
    }
    if (settings.containsKey(F("meter_mqtt_topics"))) {
      meter_mqtt_topics = settings[F("meter_mqtt_topics")].as<String>();
      initTopics(meter_mqtt_topics);
    }
  }
  void mapTopic(String &topic, String &serial) { topicMap[topic] = serial; }
  void initTopics(String &meter_topics) {
    char buffer[meter_topics.length()];
    strcpy(buffer, meter_topics.c_str());
    char *token = strtok(buffer, ";");
    while (token != NULL) {
      initTopic(token);
      token = strtok(NULL, ";");
    }
  }
  void initTopic(char *t) {
    String buffer = String(t);
    if (buffer.indexOf(':') == -1) {
      subscribeMqtt(t, false);
    } else {
      String topic = buffer.substring(0, buffer.indexOf(':'));
      String sserial =
          buffer.substring(buffer.indexOf(':') + 1, buffer.length());
      mapTopic(topic, sserial);
      subscribeMqtt((char *)topic.c_str(), false);
    }
  }
  void internalCallback(std::shared_ptr<PluginMessage> message) {

    // DBGPRINTMESSAGELNCB(DBG_INFO, getName(), message);
    if (message->isMessageType<MqttMessage>()) {
      MqttMessage *m = (MqttMessage *)message.get();
      mqttCallback(m);
    } 
  }

private:
  meterarray<MAX_NUM_METER> meters;
  std::map<String, String> topicMap;
  String meter_mqtt_topics;
  String meter_mqtt_json_topics;
};
