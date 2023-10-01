#pragma once

#include "base/plugin.h"

class DevelSupportPlugin : public Plugin {
public:
  DevelSupportPlugin() : Plugin(999, "develsupport") {}
  void setup() {}
  void onTickerSetup() {
    PDebug.printf(PDebugLevel::DEBUG, "DevelSupportPlugin:onTickerSetup()\n");
    if (debugHeap) {
      addTimerCb(
          SECOND, 10,
          [this]() {
            PDebug.printf(PDebugLevel::DEBUG,
                          "DevelSupportPlugin: free heap: %d\n",
                          ESP.getFreeHeap());
          },
          "debugHeapTimer");
    }
  }

  void internalCallback(std::shared_ptr<PluginMessage> message) {
    if (debugPluginMessages) {
      char buffer[256];
      message.get()->toString(buffer);
      PDebug.printf(PDebugLevel::INFO, "DevelSupportPlugin message: %s",
                    buffer);
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
          m.unit = Units.toUnit(request["unit"]);
          m.serial = request["serial"].as<String>();
          publishMessage(m);
          break;
        }
        case TYPEIDS::HOYMILESMESSAGE_TYPE: {
          HoymilesMessage m(*this);
          m.value = request["power"];
          m.unit = Units.toUnit(request["unit"]);
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
          if (level >= MessageOutputDebugLevel::DEBUG_NONE &&
              level <= MessageOutputDebugLevel::DEBUG_TRACE) {
            MessageOutput.setLevel((MessageOutputDebugLevel)level);
            return true;
          }
        }
        if (request.containsKey("plugins")) {
          int level = request["level"];
          if (level >= PDebugLevel::NONE && level <= PDebugLevel::TRACE) {
            PDebug.setLevel((PDebugLevel)level);
            return true;
          }
        }
      }
    }
    return false;
  }

  void saveSettings(JsonObject settings) {
    settings[F("debugHeap")] = debugHeap;
    settings[F("debugPluginMessages")] = debugPluginMessages;
  }
  void loadSettings(JsonObject settings) {
    if (settings.containsKey(F("debugHeap")))
      debugHeap = settings[F("debugHeap")];
    if (settings.containsKey(F("debugPluginMessages")))
      debugPluginMessages = settings[F("debugPluginMessages")];
  }

private:
  bool debugHeap = false;
  bool debugPluginMessages = false;
};
