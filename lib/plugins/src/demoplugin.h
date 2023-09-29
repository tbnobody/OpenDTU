#pragma once

#include "base/plugin.h"
#include "messages/demomessage.h"

class demoPlugin : public Plugin {
  enum demoPluginIds { SOMEVALUE, SOMEOTHERVALUE };

public:
  demoPlugin() : Plugin(999, "demo") {}
  void setup() {}
  void onTickerSetup() {
    PDebug.printf(PDebugLevel::DEBUG, "demoplugin:onTickerSetup()\n");
    /**   addTimerCb(
           SECOND, 5,
           [&]() {
             DemoMessage m(*this);
             m.somevalue = 08.15f;
             publishMessage(m);
             MqttMessage mqtt(getId(), PluginIds::PluginPublish);
             mqtt.setMqtt("public/mqtt", (const uint8_t *)"{hello world}", 13);
             publishMessage(mqtt);
           },
           "demoplugintimer1");

       addTimerCb(
           SECOND, 2,
           [&]() {
             DemoMessage m(*this);
             m.somevalue = 23.0f;
             publishMessage(m);
           },
           "demoplugintimer2");

       */
    if (debugHeap) {
      addTimerCb(
          SECOND, 10,
          [this]() {
            PDebug.printf(PDebugLevel::DEBUG, "demoplugin: free heap: %d\n",
                          ESP.getFreeHeap());
          },
          "debugHeapTimer");
    }
  }
  void loop() {
    // main loop
  }
  void inverterCallback(const InverterMessage *message) {
    // receice inverter data
  }

  void onMqttSubscribe() { subscribeMqtt((char *)"public/topic", false); }

  void mqttCallback(const MqttMessage *message) {

    PDebug.printf(PDebugLevel::DEBUG, "demoplugin: mqttCallback %s \n",
                  message->topic.get());
  }

  void internalCallback(std::shared_ptr<PluginMessage> message) {
    if (debugPluginMessages) {
      // PDebug.printf(PDebugLevel::DEBUG, "demoplugin", message);
    }
    if (message->isMessageType<MqttMessage>()) {
      const MqttMessage *m = (MqttMessage *)message.get();
      mqttCallback(m);
    }
  }

  bool onRequest(JsonObject request, JsonObject response) {
    if (request.containsKey("devel")) {
      if (request.containsKey("type")) {
        int type = request["type"];
        switch (type) {
        case TYPEIDS::INVERTERMESSAGE_TYPE: {
          InverterMessage m(*this);
          m.value = request["power"];
          m.inverterId = request["serial"].as<String>();
          publishMessage(m);
          break;
        }
        case TYPEIDS::METERMESSAGE_TYPE: {
          MeterMessage m(*this);
          m.power = request["power"];
          m.serial = request["serial"].as<String>();
          publishMessage(m);
          break;
        }
        case TYPEIDS::HOYMILESMESSAGE_TYPE: {
          HoymilesMessage m(*this);
          m.value = request["power"];
          m.inverterId = request["serial"].as<String>();
          publishMessage(m);
          break;
        }
        default: {
        }
        }
        return true;
      }
      if (request.containsKey("debug")) {
        if (request.containsKey("opendtu")) {
          int level = request["level"];
          switch (level) {
          case MessageOutputDebugLevel::DEBUG_NONE: {
            MessageOutput.setLevel(MessageOutputDebugLevel::DEBUG_NONE);
            break;
          }
          case MessageOutputDebugLevel::DEBUG_INFO: {
            MessageOutput.setLevel(MessageOutputDebugLevel::DEBUG_INFO);
            break;
          }
          case MessageOutputDebugLevel::DEBUG_DEBUG: {
            MessageOutput.setLevel(MessageOutputDebugLevel::DEBUG_DEBUG);
            break;
          }
          case MessageOutputDebugLevel::DEBUG_TRACE: {
            MessageOutput.setLevel(MessageOutputDebugLevel::DEBUG_TRACE);
            break;
          }
          default: {
          }
          }
          return true;
        }
        if (request.containsKey("plugins")) {
          int level = request["level"];
          switch (level) {
          case PDebugLevel::NONE: {
            PDebug.setLevel(PDebugLevel::NONE);
            break;
          }
          case PDebugLevel::INFO: {
            PDebug.setLevel(PDebugLevel::INFO);
            break;
          }
          case PDebugLevel::DEBUG: {
            PDebug.setLevel(PDebugLevel::DEBUG);
            break;
          }
          case PDebugLevel::TRACE: {
            PDebug.setLevel(PDebugLevel::TRACE);
            break;
          }
          default: {
          }
          }
          return true;
        }
      }
    }
    return false;
  }

  void saveSettings(JsonObject settings) {
    settings[F("debugHeap")] = debugHeap;
    settings[F("debugPluginMessages")] = debugPluginMessages;
    settings[F("booleansetting")] = booleansetting;
    settings[F("floatsetting")] = floatsetting;
    settings[F("stringsetting")] = stringsetting;
  }
  void loadSettings(JsonObject settings) {
    if (settings.containsKey(F("booleansetting")))
      booleansetting = settings[F("booleansetting")];
    if (settings.containsKey(F("floatsetting")))
      floatsetting = settings[F("floatsetting")];
    if (settings.containsKey(F("stringsetting")))
      settings[F("stringsetting")].as<String>().toCharArray(
          stringsetting, sizeof(stringsetting));
    if (settings.containsKey(F("debugHeap")))
      debugHeap = settings[F("debugHeap")];
    if (settings.containsKey(F("debugPluginMessages")))
      debugPluginMessages = settings[F("debugPluginMessages")];
  }

private:
  bool debugHeap = false;
  bool debugPluginMessages = false;
  bool booleansetting = false;
  float floatsetting = 23.0;
  char stringsetting[32] = "some default string";
};
