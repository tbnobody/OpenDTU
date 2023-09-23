#pragma once

#include "base/pluginmessages.h"

class DemoMessage : public PluginMessage {
public:
  DemoMessage(Plugin &p) : PluginMessage(TYPEIDS::DEMOMESSAGE_TYPE, p) {}
  int toString(char *buffer) {
    int c = sprintf(buffer, "DemoMessage{base=");
    c = c + PluginMessage::toString(buffer+c);
    c = c + sprintf(buffer+c,", somevalue=%f}",somevalue);
    return c;
  }
  float somevalue = 0;
};
template <> struct EntityIds<DemoMessage> {
  enum { type_id = TYPEIDS::DEMOMESSAGE_TYPE };
};