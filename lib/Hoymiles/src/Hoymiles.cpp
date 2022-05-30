#include "Hoymiles.h"
#include "inverters/HM_4CH.h"
#include <Arduino.h>
#include <Every.h>

HoymilesClass Hoymiles;

void HoymilesClass::init()
{
    _pollInterval = 0;
    _radio.reset(new HoymilesRadio());
    _radio->init();
}

void HoymilesClass::loop()
{
    _radio->loop();
}

std::shared_ptr<InverterAbstract> HoymilesClass::addInverter(const char* name, uint64_t serial)
{
    std::shared_ptr<InverterAbstract> i;
    if (HM_4CH::isValidSerial(serial)) {
        i = std::make_shared<HM_4CH>();
    }

    if (i) {
        i->setSerial(serial);
        i->setName(name);
        _inverters.push_back(std::move(i));
        return _inverters.back();
    }

    return nullptr;
}

std::shared_ptr<InverterAbstract> HoymilesClass::getInverterByPos(uint8_t pos)
{
    if (pos > _inverters.size()) {
        return nullptr;
    } else {
        return _inverters[pos];
    }
}

std::shared_ptr<InverterAbstract> HoymilesClass::getInverterBySerial(uint64_t serial)
{
    for (uint8_t i = 0; i < _inverters.size(); i++) {
        if (_inverters[i]->serial() == serial) {
            return _inverters[i];
        }
    }
    return nullptr;
}

void HoymilesClass::removeInverterByPos(uint8_t pos)
{
    _inverters.erase(_inverters.begin() + pos);
}

size_t HoymilesClass::getNumInverters()
{
    return _inverters.size();
}

HoymilesRadio* HoymilesClass::getRadio()
{
    return _radio.get();
}

uint32_t HoymilesClass::PollInterval()
{
    return _pollInterval;
}

void HoymilesClass::setPollInterval(uint32_t interval)
{
    _pollInterval = interval;
}