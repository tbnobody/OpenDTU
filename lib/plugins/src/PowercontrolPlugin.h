#pragma once

#include "base/plugin.h"
#include "base/pluginmessages.h"
#include "messages/invertermessage.h"
#include "messages/metermessage.h"
#include "messages/powercontrolmessage.h"
#include "base/powercontrolalgo.hpp"
#include "defaultpowercontrolalgo.hpp"

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
  enum pluginIds { INVERTER, INVERTERSTRING, POWERLIMIT };

public:
  PowercontrolPlugin() : Plugin(3, "powercontrol") {}

  void setup() {}
  void loop() {
    for (int i = 0; i < powercontrollers.size(); i++) {
      if (powercontrollers[i].update) {
        powercontrollers[i].update = false;
        if (calcLimit(powercontrollers[i])) {
          publishLimit(powercontrollers[i]);
        }
      }
    }
  }

  bool calcLimit(powercontrolstruct &powercontrol) {
    return algo->calcLimit(powercontrol);
  }
  void internalDataCallback(PluginMessage *message){}

  void publishLimit(powercontrolstruct &pc) {
    PowerControlMessage m(*this);
    m.inverterId = pc.inverterId;
    m.power = pc.limit;
    publishMessage(m);
    char topic[pc.inverterId.length() + 7];
    char payload[32];
    int len = snprintf(payload, sizeof(payload), "%f", pc.limit);
    snprintf(topic, sizeof(payload), "%s/updateLimit",
             pc.inverterId.c_str());
    MqttMessage mqtt(getId(),PluginIds::PluginPublish);
    mqtt.setMqtt(topic,(const uint8_t*)payload,len);
    mqtt.appendTopic = true;
    publishMessage(mqtt);
  }

  void handleInverterMessage(InverterMessage *message) {
    powercontrolstruct *powercontrol =
        powercontrollers.getInverterById(
            message->inverterId);
    if (powercontrol) {
      powercontrol->production = message->value;
      powercontrol->update = true;
      MessageOutput.printf("powercontrol got production: %f\n",
                           powercontrol->production);
    } else {
      MessageOutput.printf("powercontrol inverterId(%s) not configured\n",
                           message->inverterId.c_str());
    }
  }

  void handleMeterMessage(MeterMessage *m) {
    String meterserial = m->serial;
    powercontrolstruct *powercontrol =
        powercontrollers.getMeterByStringSerial(meterserial);
    if (powercontrol) {
      powercontrol->consumption = m->power;
      powercontrol->update = true;
      MessageOutput.printf("powercontrol got consumption: %f\n",
                           powercontrol->consumption);
    } else {
      MessageOutput.printf("powercontrol meterserial(%s) not configured\n",
                           meterserial.c_str());
    }
  }

  void internalCallback(std::shared_ptr<PluginMessage> message) {
    // DBGPRINTMESSAGELNCB(DBG_INFO, getName(), message);
    if (message->isMessageType<InverterMessage>()) {
      InverterMessage *m = (InverterMessage *)message.get();
      handleInverterMessage(m);
    } else if (message->isMessageType<MeterMessage>()) {
      MeterMessage *m = (MeterMessage *)message.get();
      handleMeterMessage(m);
    } else {
      MessageOutput.printf("powercontrol unhandled message from sender=%d\n",
                           message->getSenderId());
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
  DefaultPowercontrolAlgo defaultAlgo = DefaultPowercontrolAlgo();
  PowercontrolAlgo *algo = &defaultAlgo;
};
