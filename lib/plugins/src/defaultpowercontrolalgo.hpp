#pragma once

#include "base/powercontrolalgo.hpp"

class DefaultPowercontrolAlgo : public PowercontrolAlgo {
public:
  DefaultPowercontrolAlgo() : PowercontrolAlgo() {}
  bool calcLimit(powercontrolstruct &powercontrol) {
    MessageOutput.printf("powercontrol PowercontrolAlgo: consumption=%f "
                         "production=%f limit=%f\n",
                         powercontrol.consumption, powercontrol.production,
                         powercontrol.limit);
    // TODO: do some magic calculation here
    // :/
    // float newlimit =
    // magicFunction(powercontrol.production,powercontrol.consumption);

    float newLimit = powercontrol.consumption;
    float threshold = std::abs(powercontrol.limit - newLimit);
    if (threshold <= powercontrol.threshold) {
      MessageOutput.printf("powercontrol PowercontrolAlgo: newlimit(%f) within "
                           "threshold(%f) -> no limit change\n",
                           newLimit, threshold);
    } else {
      MessageOutput.printf(
          "powercontrol PowercontrolAlgo: setting limit to %f\n", newLimit);
      powercontrol.limit = newLimit;
      return true;
    }
    return false;
  }
};
