#pragma once

#include "InverterAbstract.h"

class HM_Abstract : public InverterAbstract {
public:
    HM_Abstract(uint64_t serial);
    bool getStatsRequest(inverter_transaction_t* payload);
};