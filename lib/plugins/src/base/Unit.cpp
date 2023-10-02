#include "Unit.h"
#include "PluginDebug.h"

UnitsClass Units;
const char *UnitStr[] = {"", "W", "kW", "wH", "kwH"};

const char *UnitsClass::toStr(Unit t) { return UnitStr[t]; }
Unit UnitsClass::toUnit(String &unit) { return toUnit(unit.c_str()); }
Unit UnitsClass::toUnit(const char *unit) {
  if (strcmp(UnitStr[Unit::W], unit) == 0) {
    return Unit::W;
  } else if (strcmp(UnitStr[Unit::wH], unit) == 0) {
    return Unit::wH;
  } else if (strcmp(UnitStr[Unit::kW], unit) == 0) {
    return Unit::kW;
  } else if (strcmp(UnitStr[Unit::kwH], unit) == 0) {
    return Unit::kwH;
  } else {
    return Unit::NO_UNIT;
  }
}

float UnitsClass::convert(Unit from, Unit to, float val) {
  PDebug.printf(PDebugLevel::DEBUG, "UnitsClass: convert %f from %s to %s\n",
                val, toStr(from), toStr(to));
  if (from == to) {
    return val;
  } else if (from == Unit::W && to == Unit::kW) {
    return (val / 1000);
  } else if (from == Unit::kW && to == Unit::W) {
    return (val * 1000);
  } else if (from == Unit::wH && to == Unit::kwH) {
    return (val / 1000);
  } else if (from == Unit::kwH && to == Unit::wH) {
    return (val * 1000);
  } else {
    PDebug.printf(PDebugLevel::WARN,
                  "UnitsClass: convert from %s to %s not possible!\n",
                  toStr(from), toStr(to));
    return val;
  }
}
