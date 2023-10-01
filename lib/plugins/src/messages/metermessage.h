#pragma once

#include "base/pluginmessages.h"

class MeterMessage : public PluginMessage {
public:
  MeterMessage(Plugin &p) : PluginMessage(TYPEIDS::METERMESSAGE_TYPE, p) {}
  ~MeterMessage() {}
  void setPowerValue(float power_) { power = power_; }
  float power;
  Unit unit = Unit::W;
  String serial;
  int toString(char *buffer) {
    int c = sprintf(buffer, "MeterMessage{base=");
    c = c + PluginMessage::toString(buffer + c);
    c = c +
        sprintf(buffer + c, ", meterid=%s, power=%f, unit=%s}", serial.c_str(), power, Units.toStr(unit));
    return c;
  }
  virtual const char *getMessageTypeString() { return "MeterMessage"; }
};
template <> struct EntityIds<MeterMessage> {
  enum { type_id = TYPEIDS::METERMESSAGE_TYPE };
};