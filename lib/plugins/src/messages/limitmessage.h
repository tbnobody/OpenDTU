#pragma once

#include "base/pluginmessages.h"

class LimitMessage : public PluginMessage {
public:
  LimitMessage(Plugin &p)
      : PluginMessage(TYPEIDS::LIMITMESSAGE_TYPE, p) {}
  ~LimitMessage() {}
  String deviceId;
  float limit;
  Unit unit = Unit::W;
  int toString(char *buffer) {
    int c = sprintf(buffer, "LimitMessage{base=");
    c = c + PluginMessage::toString(buffer + c);
    c = c +
        sprintf(buffer + c, ", deviceId=%s, limit=%f, unit=%s}", deviceId.c_str(), limit, Units.toStr(unit));
    return c;
  }
  virtual const char *getMessageTypeString() { return "LimitMessage"; }
};

template <> struct EntityIds<LimitMessage> {
  enum { type_id = TYPEIDS::LIMITMESSAGE_TYPE };
};