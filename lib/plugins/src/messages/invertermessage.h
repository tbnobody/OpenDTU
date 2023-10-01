#pragma once

#include "base/pluginmessages.h"

class InverterMessage : public PluginMessage {
public:
  InverterMessage(Plugin &p)
      : PluginMessage(TYPEIDS::INVERTERMESSAGE_TYPE, p) {}
  ~InverterMessage() {}
  String inverterId;
  float value;
  Unit unit = Unit::W;
  int toString(char *buffer) {
    int c = sprintf(buffer, "InverterMessage{base=");
    c = c + PluginMessage::toString(buffer + c);
    c = c +
        sprintf(buffer + c, ", id=%s, power=%f, unit=%s}", inverterId.c_str(), value, Units.toStr(unit));
    return c;
  }
  virtual const char *getMessageTypeString() { return "InverterMessage"; }
};

template <> struct EntityIds<InverterMessage> {
  enum { type_id = TYPEIDS::INVERTERMESSAGE_TYPE };
};