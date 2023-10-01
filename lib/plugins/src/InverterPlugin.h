#pragma once

#include "base/plugin.h"
#include "messages/hoymilesmessage.h"
#include "messages/hoymileslimitmessage.h"
#include "messages/invertermessage.h"
#include "messages/powercontrolmessage.h"

#ifndef MAX_NUM_INVERTERS
#define MAX_NUM_INVERTERS 5
#endif

enum class InverterEnumType { UNKNOWN = 0, HOYMILES = 1 };

typedef struct {
  InverterEnumType type = InverterEnumType::UNKNOWN;
  String inverterId;
  float actpower = 0;
  float actlimit = 0.0;
  float newlimit = 0.0;
  unsigned long limitTs = 0;
  bool updateInverter = false;
  bool updateLimit = false;
} inverterstruct;

template <std::size_t N>
class inverterarray : public structarray<inverterstruct, N> {
public:
  inverterarray() : structarray<inverterstruct, N>() {}
  inverterstruct *getInverterByInverterId(String &serial) {
    return structarray<inverterstruct, N>::getByKey(
        [&serial](inverterstruct &m) { return serial.equals(m.inverterId); });
  }
  inverterstruct *getEmptyIndex() {
    return structarray<inverterstruct, N>::getByKey(
        [](inverterstruct &s) { return s.inverterId.isEmpty(); });
  }
};

class InverterPlugin : public Plugin {

public:
  InverterPlugin() : Plugin(1, "inverter") {}

  void setup() {
    subscribe<PowerControlMessage>();
  }
  void loop() {
    for (int i = 0; i < inverters.size(); i++) {
      if (inverters[i].updateInverter) {
        inverters[i].updateInverter = false;
        publishAC(inverters[i]);
      }
      if (inverters[i].updateLimit) {
        inverters[i].updateLimit = false;
        calcLimit(inverters[i]);
      }
    }
  }
  void onTickerSetup() {}

  inverterstruct *addInverter(InverterEnumType type, const String &inverterId) {
    inverterstruct *inverter = inverters.getEmptyIndex();
    if (inverter != nullptr) {
      inverter->type = type;
      inverter->inverterId = inverterId;
    }
    return inverter;
  }

  void setInverterLimit(inverterstruct *inverter, float limit) {
    if (limit == -1) {
      PDebug.printf(PDebugLevel::DEBUG,"inverterplugin: inverter[%s] limit -1 ignored\n",
                           inverter->inverterId.c_str());
      return;
    }
    inverter->newlimit = limit;
    inverter->updateLimit = true;
  }
  void setInverterPower(inverterstruct *inverter, float power) {
    inverter->actpower = power;
    inverter->updateInverter = true;
  }
  void setLimit(inverterstruct &inverter) {

    PDebug.printf(PDebugLevel::DEBUG,"inverterplugin: setLimit %f W to %s\n",
                         inverter.newlimit, inverter.inverterId.c_str());
    if (inverter.type == InverterEnumType::HOYMILES) {
      inverter.actlimit = inverter.newlimit;
      inverter.limitTs = millis();
      // TODO: do we need a response message from hoymilesplugin
      HoymilesLimitMessage m(*this, PluginIds::PluginHoymilesinverter);
      m.inverterId = inverter.inverterId;
      m.limit = inverter.newlimit;
      publishMessage(m);
    } else {
      PDebug.printf(PDebugLevel::DEBUG,"inverterplugin: unknown inverter type: %d\n",
                           static_cast<int>(inverter.type));
    }
  }

  void calcLimit(inverterstruct &inverter) {
    PDebug.printf(PDebugLevel::DEBUG,"inverterplugin: calcLimit[%s]\n",
                         inverter.inverterId.c_str());
    setLimit(inverter);
  }

  void publishAC(inverterstruct &inverter) {
    PDebug.printf(PDebugLevel::DEBUG,"inverterplugin: publishAC[%s]: %f\n",
                         inverter.inverterId.c_str(), inverter.actpower);

    InverterMessage message(*this);
    message.inverterId = inverter.inverterId;
    message.value = inverter.actpower;
    message.unit = Unit::W;
    publishMessage(message);
  }

  void updateInverter(InverterEnumType type, String inverterId, float value) {
    PDebug.printf(PDebugLevel::DEBUG,"inverterplugin: new ac power: %f\n", value);
    inverterstruct *index = inverters.getInverterByInverterId(inverterId);
    ;
    if (index == nullptr) {
      index = addInverter(type, inverterId);
    }
    if (index == nullptr) {
      PDebug.printf(PDebugLevel::DEBUG,
          "InverterPlugin: warning! no inverter with serial %s found\n",
          inverterId.c_str());
      return;
    }
    PDebug.printf(PDebugLevel::DEBUG,"inverterplugin: update inverter.acpower[%s]: %f\n",
                         inverterId.c_str(), value);
    setInverterPower(index, value);
  }
  void internalCallback(std::shared_ptr<PluginMessage> message) {
    // DBGPRINTMESSAGELNCB(DBG_INFO, getName(), message);
    if (message->isMessageType<PowerControlMessage>()) {
      PowerControlMessage *m = (PowerControlMessage *)message.get();
      handleMessage(m);
    } else if (message->isMessageType<HoymilesMessage>()) {
      HoymilesMessage *m = (HoymilesMessage *)message.get();
      handleMessage(m);
    }
  }

  void handleMessage(HoymilesMessage *message) {
    updateInverter(InverterEnumType::HOYMILES, message->inverterId,
                   Units.convert(message->unit,Unit::W,message->value));
  }

  void handleMessage(PowerControlMessage *message) {
    String inverterId = message->inverterId;
    inverterstruct *inverter = inverters.getInverterByInverterId(inverterId);
    if (inverter != nullptr) {
      PDebug.printf(PDebugLevel::DEBUG,
          "InverterPlugin: found inverter with id %s\n",
          inverterId.c_str());
      setInverterLimit(inverter, Units.convert(message->unit,Unit::W,message->power));
    } else {
      PDebug.printf(PDebugLevel::WARN,
          "InverterPlugin: inverter(%s) not found!\n",
          inverterId.c_str());
    }
  }

private:
  inverterarray<MAX_NUM_INVERTERS> inverters;
};
