#pragma once

#include "base/pluginmessages.h"

class LimitControlMessage : public PluginMessage {
public:
  LimitControlMessage(Plugin &p)
      : PluginMessage(TYPEIDS::LIMITCONTROLMESSAGE_TYPE, p) {}
  ~LimitControlMessage() {}
  float limit;
  Unit unit = Unit::W;
  String deviceId;
  int toString(char *buffer) {
    int c = sprintf(buffer, "LimitControlMessage{base=");
    c = c + PluginMessage::toString(buffer + c);
    c = c + sprintf(buffer + c, ", deviceId=%s, limit=%f, unit=%s}",
                    deviceId.c_str(), limit, Units.toStr(unit));
    return c;
  }
  virtual const char *getMessageTypeString() { return "LimitControlMessage"; }
};
template <> struct EntityIds<LimitControlMessage> {
  enum { type_id = TYPEIDS::LIMITCONTROLMESSAGE_TYPE };
};