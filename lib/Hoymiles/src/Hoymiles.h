#pragma once

#include "HoymilesRadio.h"
#include "inverters/InverterAbstract.h"
#include "types.h"
#include <SPI.h>
#include "mp_memory.h"
#include "mp_vector.h"
#include "Clock.h"

#define HOY_SYSTEM_CONFIG_PARA_POLL_INTERVAL ((uint32_t) 10 * 60 * 1000) // 10 minutes

class HoymilesClass {
public:
    void init(_SPI* initialisedSpiBus, Clock* clock);
    void loop();

    std::shared_ptr<InverterAbstract> addInverter(const char* name, uint64_t serial);
    std::shared_ptr<InverterAbstract> getInverterByPos(uint8_t pos);
    std::shared_ptr<InverterAbstract> getInverterBySerial(uint64_t serial);
    std::shared_ptr<InverterAbstract> getInverterByFragment(fragment_t* fragment);
    void removeInverterBySerial(uint64_t serial);
    size_t getNumInverters();

    HoymilesRadio* getRadio();

    uint32_t PollInterval();
    void setPollInterval(uint32_t interval);

private:
    std::vector<std::shared_ptr<InverterAbstract>> _inverters;
    std::unique_ptr<HoymilesRadio> _radio;

    Clock* _clock;
    uint32_t _pollInterval = 0;
    uint32_t _lastPoll = 0;
};

extern HoymilesClass Hoymiles;