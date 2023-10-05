#pragma once

#include "avgpowercontrolalgo.hpp"
#include "base/plugin.h"
#include "base/pluginmessages.h"
#include "base/powercontrolalgo.hpp"
#include "defaultpowercontrolalgo.hpp"
#include "messages/limitcontrolmessage.h"
#include "messages/limitmessage.h"
#include "messages/metermessage.h"
#include "messages/mqttmessage.h"
#include "messages/powercontrolmessage.h"
#include "messages/powermessage.h"

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
    subscribe<PowerMessage>();
    subscribe<LimitMessage>();
  }
  void loop() {
    for (int i = 0; i < powercontrollers.size(); i++) {
      if (hasChanged(powercontrollers[i])) {
        PDebug.printf(PDebugLevel::DEBUG,
                      "powercontrol update: consumption=%f "
                      "production=%f limit=%f\n",
                      powercontrollers[i].consumption,
                      powercontrollers[i].production,
                      powercontrollers[i].limit);
      }
      if (powercontrollers[i].updateConsumption) {
        powercontrollers[i].updateConsumption = false;
        PDebug.printf(PDebugLevel::DEBUG,
                      "powercontrol powercontrollers[%d].update\n", i);
        if (isInitialized(powercontrollers[i]) &&
            calcLimit(powercontrollers[i])) {
          publishLimit(powercontrollers[i]);
        }
      }
      if (powercontrollers[i].updateProduction) {
        powercontrollers[i].updateProduction = false;
        // do we have to do something? :/
      }
    }
  }
  inline bool hasChanged(powercontrolstruct &powercontrol) {
    return (powercontrol.updateConsumption || powercontrol.updateProduction);
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
    int len = snprintf(payload, sizeof(payload), "%f", pc.newLimit);
    snprintf(topic, sizeof(payload), "%s/updateLimit", pc.inverterId.c_str());
    MqttMessage mqtt(getId(), PluginIds::PluginPublish);
    mqtt.setMqtt(topic, (const uint8_t *)payload, len);
    mqtt.appendTopic = true;
    publishMessage(mqtt);
  }

  void publishLimitMessage(powercontrolstruct &pc) {
    LimitControlMessage m(*this);
    m.deviceId = pc.inverterId;
    m.limit = pc.newLimit;
    m.unit = Unit::W;
    publishMessage(m);
  }

  void handleMessage(PowerMessage *m) {
    powercontrolstruct *powercontrol =
        powercontrollers.getInverterById(m->deviceId);
    if (powercontrol) {
      updateProduction(powercontrol, Units.convert(m->unit, Unit::W, m->value),
                       m->getTS());
    } else {
      PDebug.printf(PDebugLevel::WARN,
                    "powercontrol inverterId(%s) not configured\n",
                    m->deviceId.c_str());
    }
  }

  void handleMessage(LimitMessage *m) {
    powercontrolstruct *powercontrol =
        powercontrollers.getInverterById(m->deviceId);
    if (powercontrol) {
      updateLimit(powercontrol, Units.convert(m->unit, Unit::W, m->limit),
                  m->getTS());
    } else {
      PDebug.printf(PDebugLevel::WARN,
                    "powercontrol inverterId(%s) not configured\n",
                    m->deviceId.c_str());
    }
  }

  void updateLimit(powercontrolstruct *powercontrol, float power,
                   unsigned long ts) {
    powercontrol->limit = power;
    powercontrol->limitTs = ts;
    PDebug.printf(PDebugLevel::DEBUG, "powercontrol update limit: %f\n",
                  powercontrol->production);
  }

  void updateProduction(powercontrolstruct *powercontrol, float power,
                        unsigned long ts) {
    powercontrol->production = power;
    powercontrol->productionTs = ts;
    powercontrol->updateProduction = true;
    PDebug.printf(PDebugLevel::DEBUG, "powercontrol update production: %f\n",
                  powercontrol->production);
  }

  void handleMessage(MeterMessage *m) {
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
    powercontrol->updateConsumption = true;
    PDebug.printf(PDebugLevel::DEBUG, "powercontrol update consumption: %f\n",
                  powercontrol->consumption);
  }

  void internalCallback(std::shared_ptr<PluginMessage> message) {
    if (message->isMessageType<PowerMessage>()) {
      PowerMessage *m = (PowerMessage *)message.get();
      handleMessage(m);
    } else if (message->isMessageType<MeterMessage>()) {
      MeterMessage *m = (MeterMessage *)message.get();
      handleMessage(m);
    } else if (message->isMessageType<LimitMessage>()) {
      LimitMessage *m = (LimitMessage *)message.get();
      handleMessage(m);
    }
  }

  void initPowercontrol() {
    powercontrolstruct *powercontrol = powercontrollers.getEmptyIndex();
    if (powercontrol) {
      powercontrol->inverterId = inverter_serial;
      powercontrol->meterSerial = meter_serial;
      powercontrol->updateConsumption = false;
      powercontrol->updateProduction = false;
      powercontrol->limit = 0;
      powercontrol->newLimit = 0;
      powercontrol->production = 0;
      powercontrol->consumption = 0;
      powercontrol->threshold = threshold;
      powercontrol->maxDiffW = maxDiffW;
    }
  }
  void saveSettings(JsonObject settings) {
    settings[F("meter_serial")] = meter_serial;
    settings[F("inverter_serial")] = inverter_serial;
    settings[F("threshold")] = threshold;
    settings[F("max_diff_w")] = maxDiffW;
  }
  void loadSettings(JsonObject settings) {
    if (settings.containsKey(F("meter_serial"))) {
      meter_serial = settings[F("meter_serial")].as<String>();
    }
    if (settings.containsKey(F("inverter_serial"))) {
      inverter_serial = settings[F("inverter_serial")].as<String>();
    }
    if (settings.containsKey(F("threshold"))) {
      threshold = settings[F("threshold")];
    }
    if (settings.containsKey(F("max_diff_w"))) {
      maxDiffW = settings[F("max_diff_w")];
    }
    initPowercontrol();
  }

private:
  String inverter_serial = "";
  String meter_serial = "";
  int threshold = 20;
  int maxDiffW = 40;
  // powercontrolstruct powercontrol;
  powercontrollerarray<MAX_NUM_INVERTERS> powercontrollers;
  PowercontrolAlgo *algo = new AvgPowercontrolAlgo();
};
