#pragma once

#include "base/pluginmessages.h"

class PowerMessage : public PluginMessage {
public:
  PowerMessage(Plugin &p)
      : PluginMessage(TYPEIDS::POWERMESSAGE_TYPE, p) {}
  ~PowerMessage() {}
  String deviceId;
  float value;
  Unit unit = Unit::W;
  int toString(char *buffer) {
    int c = sprintf(buffer, "PowerMessage{base=");
    c = c + PluginMessage::toString(buffer + c);
    c = c +
        sprintf(buffer + c, ", deviceId=%s, value=%f, unit=%s}", deviceId.c_str(), value, Units.toStr(unit));
    return c;
  }
  virtual const char *getMessageTypeString() { return "PowerMessage"; }
};

template <> struct EntityIds<PowerMessage> {
  enum { type_id = TYPEIDS::POWERMESSAGE_TYPE };
};