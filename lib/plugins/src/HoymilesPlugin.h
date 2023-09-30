#pragma once

#include "base/plugin.h"
#include "messages/hoymileslimitmessage.h"
#include "messages/hoymilesmessage.h"
#include <Hoymiles.h>

#ifndef MAX_NUM_INVERTERS
#define MAX_NUM_INVERTERS 5
#endif

class HoymilesPlugin : public Plugin {

public:
  HoymilesPlugin() : Plugin(20, "hoymilesinverter") {}

  void setup() {}
  void loop() {}
  void onTickerSetup() {
    addTimerCb(
        SECOND, 5, [this]() { loopInverters(); }, "HoymilesloopInvertersTimer");
  }

  void loopInverters() {
    // copied from WebApi_prometheus.cpp
    // TODO: polling is bad.
    // notify on update would be fine!
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
      auto inv = Hoymiles.getInverterByPos(i);
      PDebug.printf(PDebugLevel::DEBUG,
                    "hoymilesplugin: loopInverters inv[%d] lastupdate=%d\n", i,
                    inv->Statistics()->getLastUpdate());
      if (inv->Statistics()->getLastUpdate() > 0) {
        for (auto &t : inv->Statistics()->getChannelTypes()) {
          for (auto &c : inv->Statistics()->getChannelsByType(t)) {
            publishField(inv, t, c, FLD_PAC);
            publishField(inv, t, c, FLD_UAC);
            publishField(inv, t, c, FLD_IAC);
            publishField(inv, t, c, FLD_PDC);
            publishField(inv, t, c, FLD_UDC);
            publishField(inv, t, c, FLD_IDC);
            publishField(inv, t, c, FLD_YD);
            publishField(inv, t, c, FLD_YT);
            publishField(inv, t, c, FLD_F);
            publishField(inv, t, c, FLD_T);
            publishField(inv, t, c, FLD_PF);
            publishField(inv, t, c, FLD_Q);
            publishField(inv, t, c, FLD_EFF);
            publishField(inv, t, c, FLD_IRR);
          }
        }
      }
    }
  }

  void publishField(std::shared_ptr<InverterAbstract> inv, ChannelType_t &type,
                    ChannelNum_t &channel, FieldId_t fieldId) {
    // hack !!!!!
    if (fieldId != FieldId_t::FLD_PAC) {
      return;
    }
    if (inv->Statistics()->hasChannelFieldValue(type, channel, fieldId)) {
      String value = String(
          inv->Statistics()->getChannelFieldValue(type, channel, fieldId),
          static_cast<unsigned int>(inv->Statistics()->getChannelFieldDigits(
              type, channel, fieldId)));
      value.trim();

      // use internal callback
      inverterCallback(fieldId, channel, inv->serial(), inv->serialString(),
                       value.toFloat());
    }
  }

  void setLimit(String &inverterId, float limit) {
    std::shared_ptr<InverterAbstract> inv = nullptr;
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
      auto serachinv = Hoymiles.getInverterByPos(i);
      if (serachinv->serialString().equals(inverterId)) {
        inv = serachinv;
        break;
      }
    }
    if (!allow_limitation) {
      PDebug.printf(PDebugLevel::INFO,
                    "hoymilesplugin: inverter[%s] set powerlimit not allowed\n",
                    inv->serialString().c_str());
      return;
    }
    if (limit < limit_minimum_w) {
      limit = limit_minimum_w;
      PDebug.printf(PDebugLevel::INFO,
                    "hoymilesplugin: inverter[%s] minimum limit is %f W\n",
                    inv->serialString().c_str(), limit_minimum_w);
    }
    if (inv != nullptr) {
      PDebug.printf(
          PDebugLevel::INFO,
          "hoymilesplugin: sendActivePowerControlRequest %f W to %s\n", limit,
          inv->serialString().c_str());
    } else {
      PDebug.printf(PDebugLevel::WARN,
                    "hoymilesplugin: inverter(%s) not found!\n",
                    inverterId.c_str());
    }
    if (inv != nullptr) {
      if (inv->sendActivePowerControlRequest(
              limit, PowerLimitControlType::AbsolutNonPersistent)) {
        PDebug.printf(
            PDebugLevel::DEBUG,
            "hoymilesplugin: sendActivePowerControlRequest %f W to %s -> OK!\n",
            limit, inv->serialString().c_str());
      } else {
        PDebug.printf(PDebugLevel::DEBUG,
                      "hoymilesplugin: sendActivePowerControlRequest %f "
                      "W to %s -> FAILED!\n",
                      limit, inv->serialString().c_str());
      }
    }
  }

  void publishAC(uint64_t serial, String serialString, float actpower) {
    PDebug.printf(PDebugLevel::DEBUG, "hoymilesplugin: publishAC[%s]: %f\n",
                  serialString.c_str(), actpower);

    HoymilesMessage message(*this);
    message.inverterId = serialString;
    message.value = actpower;
    publishMessage(message);
  }
  void inverterCallback(int fieldId, int channelNumber, uint64_t inverterSerial,
                        String inverterStringSerial, float value) {
    if (fieldId == FieldId_t::FLD_PAC && channelNumber == ChannelNum_t::CH0) {
      PDebug.printf(PDebugLevel::DEBUG, "hoymilesplugin: new ac power: %f\n",
                    value);
      publishAC(inverterSerial, inverterStringSerial, value);
    }
  }
  void handleMessage(HoymilesLimitMessage *message) {
    setLimit(message->inverterId, message->limit);
  }
  void internalCallback(std::shared_ptr<PluginMessage> message) {
    // DBGPRINTMESSAGELNCB(DBG_INFO, getName(), message);
    PDebug.printf(PDebugLevel::DEBUG, "hoymilesplugin: internalCallback: %s\n",
                  message.get()->getMessageTypeString());
    if (message->isMessageType<HoymilesLimitMessage>()) {
      HoymilesLimitMessage *m = (HoymilesLimitMessage *)message.get();
      handleMessage(m);
    }
  }

  void saveSettings(JsonObject settings) {
    settings[F("allow_limitation")] = allow_limitation;
    settings[F("limit_minimum_w")] = limit_minimum_w;
  }

  void loadSettings(JsonObject settings) {
    if (settings.containsKey(F("allow_limitation")))
      allow_limitation = settings[F("allow_limitation")];
    if (settings.containsKey(F("limit_minimum_w")))
      limit_minimum_w = settings[F("limit_minimum_w")];
  }

private:
  float limit_minimum_w = 50;
  bool allow_limitation = false;
};
