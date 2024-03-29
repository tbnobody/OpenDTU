#pragma once

#include <Arduino.h>
#include "VeDirectData.h"
#include "VeDirectFrameHandler.h"

class VeDirectShuntController : public VeDirectFrameHandler<veShuntStruct> {
public:
    VeDirectShuntController() = default;

    void init(int8_t rx, int8_t tx, Print* msgOut, bool verboseLogging);

    using data_t = veShuntStruct;

private:
    bool processTextDataDerived(std::string const& name, std::string const& value) final;
};

extern VeDirectShuntController VeDirectShunt;
