#pragma once

#include "base/plugin.h"
#include "base/pluginmessages.h"
#include "base/powercontrolalgo.hpp"
#include "defaultpowercontrolalgo.hpp"
#include "messages/invertermessage.h"
#include "messages/metermessage.h"
#include "messages/powercontrolmessage.h"

template <std::size_t N>
class powercontrollerarray : public structarray<powercontrolstruct, N> {
public:
  powercontrollerarray() : structarray<powercontrolstruct, N>() {}
  powercontrolstruct *getInverterById(String &serial) {
    return structarray<powercontrolstruct, N>::getByKey(
        [&serial](powercontrolstruct &pc) {
          return serial.equals(pc.inverterId);
        });
  }
  powercontrolstruct *getMeterByStringSerial(String &serial) {
    return structarray<powercontrolstruct, N>::getByKey(
        [&serial](powercontrolstruct &pc) {
          return serial.equals(pc.meterSerial);
        });
  }
  powercontrolstruct *getEmptyIndex() {
    return structarray<powercontrolstruct, N>::getByKey(
        [](powercontrolstruct &s) { return s.inverterId.isEmpty(); });
  }
};

class PowercontrolPlugin : public Plugin {

public:
  PowercontrolPlugin() : Plugin(3, "powercontrol") {}

  void setup() {
    subscribe<MeterMessage>();
    subscribe<InverterMessage>();
  }
  void loop() {
    for (int i = 0; i < powercontrollers.size(); i++) {
      if (powercontrollers[i].update) {
        powercontrollers[i].update = false;
        PDebug.printf(PDebugLevel::DEBUG,
                      "powercontrol powercontrollers[%d].update\n", i);
        if (isInitialized(powercontrollers[i]) &&
            calcLimit(powercontrollers[i])) {
          publishLimit(powercontrollers[i]);
        }
      }
    }
  }

  inline bool isInitialized(powercontrolstruct &powercontrol) {
    return (powercontrol.consumptionTs != 0 && powercontrol.productionTs != 0);
  }

  inline bool calcLimit(powercontrolstruct &powercontrol) {
    return algo->calcLimit(powercontrol);
  }

  void publishLimit(powercontrolstruct &pc) {
    publishLimitMessage(pc);
    publishLimitMqtt(pc);
  }

  void publishLimitMqtt(powercontrolstruct &pc) {
    char topic[pc.inverterId.length() + 7];
    char payload[32];
    int len = snprintf(payload, sizeof(payload), "%f", pc.limit);
    snprintf(topic, sizeof(payload), "%s/updateLimit", pc.inverterId.c_str());
    MqttMessage mqtt(getId(), PluginIds::PluginPublish);
    mqtt.setMqtt(topic, (const uint8_t *)payload, len);
    mqtt.appendTopic = true;
    publishMessage(mqtt);
  }

  void publishLimitMessage(powercontrolstruct &pc) {
    PowerControlMessage m(*this);
    m.inverterId = pc.inverterId;
    m.power = pc.limit;
    m.unit = Unit::W;
    publishMessage(m);
  }

  void handleInverterMessage(InverterMessage *m) {
    powercontrolstruct *powercontrol =
        powercontrollers.getInverterById(m->inverterId);
    if (powercontrol) {
      updateProduction(powercontrol, Units.convert(m->unit, Unit::W, m->value),
                       m->getTS());
    } else {
      PDebug.printf(PDebugLevel::WARN,
                    "powercontrol inverterId(%s) not configured\n",
                    m->inverterId.c_str());
    }
  }

  void updateProduction(powercontrolstruct *powercontrol, float power,
                        unsigned long ts) {
    powercontrol->production = power;
    powercontrol->productionTs = ts;
    powercontrol->update = true;
    PDebug.printf(PDebugLevel::DEBUG, "powercontrol update production: %f\n",
                  powercontrol->production);
  }

  void handleMeterMessage(MeterMessage *m) {
    String meterserial = m->serial;
    powercontrolstruct *powercontrol =
        powercontrollers.getMeterByStringSerial(meterserial);
    if (powercontrol) {
      updateConsumption(powercontrol, Units.convert(m->unit, Unit::W, m->power),
                        m->getTS());
    } else {
      PDebug.printf(PDebugLevel::WARN,
                    "powercontrol meterserial(%s) not configured\n",
                    meterserial.c_str());
    }
  }

  void updateConsumption(powercontrolstruct *powercontrol, float consumption,
                         unsigned long ts) {
    powercontrol->consumption = consumption;
    powercontrol->consumptionTs = ts;
    powercontrol->update = true;
    PDebug.printf(PDebugLevel::DEBUG, "powercontrol update consumption: %f\n",
                  powercontrol->consumption);
  }

  void internalCallback(std::shared_ptr<PluginMessage> message) {
    if (message->isMessageType<InverterMessage>()) {
      InverterMessage *m = (InverterMessage *)message.get();
      handleInverterMessage(m);
    } else if (message->isMessageType<MeterMessage>()) {
      MeterMessage *m = (MeterMessage *)message.get();
      handleMeterMessage(m);
    }
  }

  void initPowercontrol() {
    powercontrolstruct *powercontrol = powercontrollers.getEmptyIndex();
    if (powercontrol) {
      powercontrol->inverterId = inverter_serial;
      powercontrol->meterSerial = meter_serial;
      powercontrol->update = false;
      powercontrol->limit = 0;
      powercontrol->threshold = threshold;
    }
  }
  void saveSettings(JsonObject settings) {
    settings[F("meter_serial")] = meter_serial;
    settings[F("inverter_serial")] = inverter_serial;
    settings[F("threshold")] = threshold;
  }
  void loadSettings(JsonObject settings) {
    if (settings.containsKey(F("meter_serial"))) {
      meter_serial = settings[F("meter_serial")].as<String>();
    }
    if (settings.containsKey(F("inverter_serial"))) {
      inverter_serial = settings[F("inverter_serial")].as<String>();
    }
    if (settings.containsKey(F("threshold"))) {
      threshold = settings[F("threshold")].as<uint32_t>();
    }
    initPowercontrol();
  }

private:
  String inverter_serial;
  String meter_serial;
  uint32_t threshold = 20;
  // powercontrolstruct powercontrol;
  powercontrollerarray<MAX_NUM_INVERTERS> powercontrollers;
  PowercontrolAlgo *algo = new DefaultPowercontrolAlgo();
};
