#pragma once

#include "base/PluginDebug.h"
#include "base/powercontrolalgo.hpp"
#include <array>

#define ARRAYSIZE 2
typedef struct {
  int input[ARRAYSIZE];
  int output[ARRAYSIZE];
} History;

class AvgPowercontrolAlgo : public PowercontrolAlgo {

public:
  AvgPowercontrolAlgo() : PowercontrolAlgo(), index(0), initialized(false) {
    init();
  }
  ~AvgPowercontrolAlgo() {}
  virtual bool calcLimit(powercontrolstruct &powercontrol) {

    float newLimit = initialized ? getLimit(powercontrol.consumption,powercontrol.maxDiffW)
                                 : powercontrol.consumption;
    saveHistory(powercontrol.consumption, newLimit);
    float threshold = std::abs(powercontrol.limit - newLimit);
    if (threshold <= powercontrol.threshold) {
      PDebug.printf(PDebugLevel::DEBUG,
                    "powercontrol AvgPowercontrolAlgo: newlimit(%f) within "
                    "threshold(%f) -> no limit change\n",
                    newLimit, threshold);
    } else {
      PDebug.printf(PDebugLevel::DEBUG,
                    "powercontrol AvgPowercontrolAlgo: setting limit to %f\n",
                    newLimit);
      powercontrol.newLimit = newLimit;
      return true;
    }
    return false;
  }

private:
  void init() {
    for (int i = 0; i < ARRAYSIZE; i++) {
      history.input[i] = 0;
      history.output[i] = 0;
    }
  }
  void saveHistory(int in, int out) {
    history.input[index] = in;
    history.output[index] = out;
    index++;
    index = (index % ARRAYSIZE);
    if (!initialized && index == 0)
      initialized = true;
  }
  int getHistorySumIn() {
    int sum = 0;
    for (int i = 0; i < ARRAYSIZE; i++)
      sum += history.input[i];
    return sum;
  }
  int getHistorySumOut() {
    int sum = 0;
    for (int i = 0; i < ARRAYSIZE; i++)
      sum += history.output[i];
    return sum;
  }
  float getLimit(int in, int maxDiffW) {
    int avgIn = ((getHistorySumIn()) / (ARRAYSIZE));
    int diff = std::abs(avgIn - in);
    if (diff > maxDiffW)
      return ((avgIn + in) / 2);
    else
      return in;
  }
  History history;
  int index;
  bool initialized;
};
