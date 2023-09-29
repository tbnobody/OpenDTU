#pragma once

#include "base/pluginmessages.h"

class HoymilesMessage : public PluginMessage {
public:
  HoymilesMessage(Plugin &p)
      : PluginMessage(TYPEIDS::HOYMILESMESSAGE_TYPE, p) {}
  ~HoymilesMessage() {}
  String inverterId;
  float value;
  int toString(char *buffer) {
    int c = sprintf(buffer, "HoymilesMessage{base=");
    c = c + PluginMessage::toString(buffer + c);
    c = c +
        sprintf(buffer + c, ", id=%s, power=%f}", inverterId.c_str(), value);
    return c;
  }
  virtual const char *getMessageTypeString() { return "HoymilesMessage"; }
};

template <> struct EntityIds<HoymilesMessage> {
  enum { type_id = TYPEIDS::HOYMILESMESSAGE_TYPE };
};
