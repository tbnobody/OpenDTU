#pragma once

typedef struct {
  String inverterId;
  String meterSerial;
  float limit = 0.0;
  float newLimit = 0.0;
  int threshold = 0;
  int maxDiffW = 0;
  float consumption = 0.0;
  unsigned long consumptionTs = 0;
  unsigned long productionTs = 0;
  unsigned long limitTs = 0;
  float production = 0.0;
  bool updateConsumption = false;
  bool updateProduction = false;
} powercontrolstruct;

class PowercontrolAlgo {
public:
  PowercontrolAlgo() {}
  virtual ~PowercontrolAlgo() {}
  virtual bool calcLimit(powercontrolstruct &powercontrol) { return false; };
};
