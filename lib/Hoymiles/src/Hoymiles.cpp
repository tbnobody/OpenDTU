#include "Hoymiles.h"
#include "inverters/HM_1CH.h"
#include "inverters/HM_2CH.h"
#include "inverters/HM_4CH.h"
#include <Arduino.h>

#define HOY_SEMAPHORE_TAKE() xSemaphoreTake(_xSemaphore, portMAX_DELAY)
#define HOY_SEMAPHORE_GIVE() xSemaphoreGive(_xSemaphore)

HoymilesClass Hoymiles;

void HoymilesClass::init()
{
    _xSemaphore = xSemaphoreCreateMutex();
    HOY_SEMAPHORE_GIVE();  // release before first use

    _pollInterval = 0;
    _radio.reset(new HoymilesRadio());
    _radio->init();
}

void HoymilesClass::loop()
{
    HOY_SEMAPHORE_TAKE();
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
                bool force = iv->EventLog()->getLastAlarmRequestSuccess() == CMD_NOK;
                iv->sendAlarmLogRequest(_radio.get(), force);

                // Fetch limit
                if ((iv->SystemConfigPara()->getLastLimitRequestSuccess() == CMD_NOK)
                    || ((millis() - iv->SystemConfigPara()->getLastUpdateRequest() > HOY_SYSTEM_CONFIG_PARA_POLL_INTERVAL)
                        && (millis() - iv->SystemConfigPara()->getLastUpdateCommand() > HOY_SYSTEM_CONFIG_PARA_POLL_MIN_DURATION))) {
                    Serial.println("Request SystemConfigPara");
                    iv->sendSystemConfigParaRequest(_radio.get());
                }

                // Set limit if required
                if (iv->SystemConfigPara()->getLastLimitCommandSuccess() == CMD_NOK) {
                    Serial.println(F("Resend ActivePowerControl"));
                    iv->resendActivePowerControlRequest(_radio.get());
                }

                // Set power status if required
                if (iv->PowerCommand()->getLastPowerCommandSuccess() == CMD_NOK) {
                    Serial.println(F("Resend PowerCommand"));
                    iv->resendPowerControlRequest(_radio.get());
                }

                // Fetch dev info (but first fetch stats)
                if (iv->Statistics()->getLastUpdate() > 0 && (iv->DevInfo()->getLastUpdateAll() == 0 || iv->DevInfo()->getLastUpdateSimple() == 0)) {
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

    HOY_SEMAPHORE_GIVE();
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
            HOY_SEMAPHORE_TAKE();
            _inverters.erase(_inverters.begin() + i);
            HOY_SEMAPHORE_GIVE();
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