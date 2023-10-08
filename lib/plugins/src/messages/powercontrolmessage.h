#pragma once

#include "base/pluginmessages.h"

class PowerControlMessage : public PluginMessage {
public:
  PowerControlMessage(Plugin &p)
      : PluginMessage(TYPEIDS::POWERCONTROLMESSAGE_TYPE, p) {}
  ~PowerControlMessage() {}
  float power;
  Unit unit = Unit::W;
  String deviceId;
  int toString(char *buffer) {
    int c = sprintf(buffer, "PowerControlMessage{base=");
    c = c + PluginMessage::toString(buffer + c);
    c = c + sprintf(buffer + c, ", deviceId=%s, power=%f, unit=%s}",
                    deviceId.c_str(), power, Units.toStr(unit));
    return c;
  }
  virtual const char *getMessageTypeString() { return "PowerControlMessage"; }
};
template <> struct EntityIds<PowerControlMessage> {
  enum { type_id = TYPEIDS::POWERCONTROLMESSAGE_TYPE };
};