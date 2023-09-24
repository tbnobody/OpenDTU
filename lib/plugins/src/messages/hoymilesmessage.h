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
    c = c + PluginMessage::toString(buffer+c);
    c = c + sprintf(buffer+c,", id=%s, power=%f}",inverterId.c_str(),value);
    return c;
  }
};

template <> struct EntityIds<HoymilesMessage> {
  enum { type_id = TYPEIDS::HOYMILESMESSAGE_TYPE };
};

class HoymilesLimitMessage : public PluginMessage {
public:
  HoymilesLimitMessage(Plugin &p)
      : PluginMessage(TYPEIDS::HOYMILESLIMITMESSAGE_TYPE, p) {}
HoymilesLimitMessage(Plugin &p, int receiverId)
      : PluginMessage(TYPEIDS::HOYMILESLIMITMESSAGE_TYPE, p) {
        setReceiverId(receiverId);
      }
  ~HoymilesLimitMessage() {}
  String inverterId;
  float limit;
  int toString(char *buffer) {
    int c = sprintf(buffer, "HoymilesLimitMessage{base=");
    c = c + PluginMessage::toString(buffer+c);
    c = c + sprintf(buffer+c,", id=%s, limit=%f}",inverterId.c_str(),limit);
    return c;
  }
};

template <> struct EntityIds<HoymilesLimitMessage> {
  enum { type_id = TYPEIDS::HOYMILESLIMITMESSAGE_TYPE };
};