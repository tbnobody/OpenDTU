#pragma once

#include "WString.h"

enum Unit { NO_UNIT, W, kW, wH, kwH };


class UnitsClass {
public:
  const char *toStr(Unit t);
  Unit toUnit(String &unit);
  Unit toUnit(const char *unit);
  float convert(Unit from, Unit to, float val);
};
extern UnitsClass Units;
