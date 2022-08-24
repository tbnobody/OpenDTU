#include "Hoymiles.h"
#include "inverters/HM_1CH.h"
#include "inverters/HM_2CH.h"
#include "inverters/HM_4CH.h"
#include <Arduino.h>

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

    if (getNumInverters() > 0) {
        if (millis() - _lastPoll > (_pollInterval * 1000)) {
            static uint8_t inverterPos = 0;

            std::shared_ptr<InverterAbstract> iv = getInverterByPos(inverterPos);
            if (iv != nullptr && _radio->isIdle()) {
                Serial.print(F("Fetch inverter: "));
                Serial.println(iv->serial(), HEX);

                iv->sendStatsRequest(_radio.get());

                // Fetch event log
                iv->sendAlarmLogRequest(_radio.get());

                // Fetch dev info (but first fetch stats)
                if (iv->Statistics()->getLastUpdate() > 0 && (iv->DevInfo()->getLastUpdateAll() == 0 || iv->DevInfo()->getLastUpdateSample() == 0)) {
                    Serial.println(F("Request device info"));
                    iv->sendDevInfoRequest(_radio.get());
                }
            }

            if (++inverterPos >= getNumInverters()) {
                inverterPos = 0;
            }

            _lastPoll = millis();
        }
    }
}

std::shared_ptr<InverterAbstract> HoymilesClass::addInverter(const char* name, uint64_t serial)
{
    std::shared_ptr<InverterAbstract> i = nullptr;
    if (HM_4CH::isValidSerial(serial)) {
        i = std::make_shared<HM_4CH>(serial);
    } else if (HM_2CH::isValidSerial(serial)) {
        i = std::make_shared<HM_2CH>(serial);
    } else if (HM_1CH::isValidSerial(serial)) {
        i = std::make_shared<HM_1CH>(serial);
    }

    if (i) {
        i->setName(name);
        i->init();
        _inverters.push_back(std::move(i));
        return _inverters.back();
    }

    return nullptr;
}

std::shared_ptr<InverterAbstract> HoymilesClass::getInverterByPos(uint8_t pos)
{
    if (pos >= _inverters.size()) {
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

std::shared_ptr<InverterAbstract> HoymilesClass::getInverterByFragment(fragment_t* fragment)
{
    if (fragment->len <= 4) {
        return nullptr;
    }

    std::shared_ptr<InverterAbstract> inv;
    for (uint8_t i = 0; i < _inverters.size(); i++) {
        inv = _inverters[i];
        serial_u p;
        p.u64 = inv->serial();

        if ((p.b[3] == fragment->fragment[1])
            && (p.b[2] == fragment->fragment[2])
            && (p.b[1] == fragment->fragment[3])
            && (p.b[0] == fragment->fragment[4])) {

            return inv;
        }
    }
    return nullptr;
}

void HoymilesClass::removeInverterBySerial(uint64_t serial)
{
    for (uint8_t i = 0; i < _inverters.size(); i++) {
        if (_inverters[i]->serial() == serial) {
            _inverters.erase(_inverters.begin() + i);
            return;
        }
    }
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