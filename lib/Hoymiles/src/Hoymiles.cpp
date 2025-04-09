// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2025 Thomas Basler and others
 */
#include "Hoymiles.h"
#include "Utils.h"
#include "inverters/HERF_1CH.h"
#include "inverters/HERF_2CH.h"
#include "inverters/HERF_4CH.h"
#include "inverters/HMS_1CH.h"
#include "inverters/HMS_1CHv2.h"
#include "inverters/HMS_2CH.h"
#include "inverters/HMS_4CH.h"
#include "inverters/HMT_4CH.h"
#include "inverters/HMT_6CH.h"
#include "inverters/HM_1CH.h"
#include "inverters/HM_2CH.h"
#include "inverters/HM_4CH.h"
#include <Arduino.h>

HoymilesClass Hoymiles;

void HoymilesClass::init()
{
    _pollInterval = 0;
    _radioNrf.reset(new HoymilesRadio_NRF());
    _radioCmt.reset(new HoymilesRadio_CMT());
}

void HoymilesClass::initNRF(SPIClass* initialisedSpiBus, const uint8_t pinCE, const uint8_t pinIRQ)
{
    _radioNrf->init(initialisedSpiBus, pinCE, pinIRQ);
}

void HoymilesClass::initCMT(const int8_t pin_sdio, const int8_t pin_clk, const int8_t pin_cs, const int8_t pin_fcs, const int8_t pin_gpio2, const int8_t pin_gpio3)
{
    _radioCmt->init(pin_sdio, pin_clk, pin_cs, pin_fcs, pin_gpio2, pin_gpio3);
}

void HoymilesClass::loop()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _radioNrf->loop();
    _radioCmt->loop();

    if (getNumInverters() == 0 || millis() - _lastPoll <= (_pollInterval * 1000)) {
        return;
    }

    static uint8_t inverterPos = 0;

    std::shared_ptr<InverterAbstract> iv = getInverterByPos(inverterPos);
    if ((iv == nullptr) || ((iv != nullptr) && (!iv->getRadio()->isInitialized()))) {
        if (++inverterPos >= getNumInverters()) {
            inverterPos = 0;
        }
    }

    if (iv != nullptr && iv->getRadio()->isInitialized()) {

        if (iv->getZeroValuesIfUnreachable() && !iv->isReachable()) {
            iv->Statistics()->zeroRuntimeData();
        }

        if (iv->getEnablePolling() || iv->getEnableCommands()) {
            _messageOutput->print("Fetch inverter: ");
            _messageOutput->println(iv->serial(), HEX);

            if (!iv->isReachable()) {
                iv->sendChangeChannelRequest();
            }

            if (Utils::getTimeAvailable()) {
                // Fetch statistics
                iv->sendStatsRequest();

                // Fetch event log
                const bool force = iv->EventLog()->getLastAlarmRequestSuccess() == CMD_NOK;
                iv->sendAlarmLogRequest(force);

                // Fetch limit
                if (((millis() - iv->SystemConfigPara()->getLastUpdateRequest() > HOY_SYSTEM_CONFIG_PARA_POLL_INTERVAL)
                        && (millis() - iv->SystemConfigPara()->getLastUpdateCommand() > HOY_SYSTEM_CONFIG_PARA_POLL_MIN_DURATION))) {
                    _messageOutput->println("Request SystemConfigPara");
                    iv->sendSystemConfigParaRequest();
                }

                // Fetch grid profile
                if (iv->Statistics()->getLastUpdate() > 0 && (iv->GridProfile()->getLastUpdate() == 0 || !iv->GridProfile()->containsValidData())) {
                    iv->sendGridOnProFileParaRequest();
                }

                // Fetch dev info (but first fetch stats)
                if (iv->Statistics()->getLastUpdate() > 0) {
                    const bool invalidDevInfo = !iv->DevInfo()->containsValidData()
                        && iv->DevInfo()->getLastUpdateAll() > 0
                        && iv->DevInfo()->getLastUpdateSimple() > 0;

                    if (invalidDevInfo) {
                        _messageOutput->println("DevInfo: No Valid Data");
                    }

                    if ((iv->DevInfo()->getLastUpdateAll() == 0)
                        || (iv->DevInfo()->getLastUpdateSimple() == 0)
                        || invalidDevInfo) {
                        _messageOutput->println("Request device info");
                        iv->sendDevInfoRequest();
                    }
                }
            }

            // Set limit if required
            if (iv->SystemConfigPara()->getLastLimitCommandSuccess() == CMD_NOK) {
                _messageOutput->println("Resend ActivePowerControl");
                iv->resendActivePowerControlRequest();
            }

            // Set power status if required
            if (iv->PowerCommand()->getLastPowerCommandSuccess() == CMD_NOK) {
                _messageOutput->println("Resend PowerCommand");
                iv->resendPowerControlRequest();
            }

            _messageOutput->printf("Queue size - NRF: %" PRIu32 " CMT: %" PRIu32 "\r\n", _radioNrf->getQueueSize(), _radioCmt->getQueueSize());
            _lastPoll = millis();
        }

        if (++inverterPos >= getNumInverters()) {
            inverterPos = 0;
        }
    }

    // Perform housekeeping of all inverters on day change
    const int8_t currentWeekDay = Utils::getWeekDay();
    static int8_t lastWeekDay = -1;
    if (lastWeekDay == -1) {
        lastWeekDay = currentWeekDay;
    } else {
        if (currentWeekDay != lastWeekDay) {

            for (auto& inv : _inverters) {
                inv->performDailyTask();
            }

            lastWeekDay = currentWeekDay;
        }
    }
}

std::shared_ptr<InverterAbstract> HoymilesClass::addInverter(const char* name, const uint64_t serial)
{
    std::shared_ptr<InverterAbstract> i = nullptr;
    if (HMT_4CH::isValidSerial(serial)) {
        i = std::make_shared<HMT_4CH>(_radioCmt.get(), serial);
    } else if (HMT_6CH::isValidSerial(serial)) {
        i = std::make_shared<HMT_6CH>(_radioCmt.get(), serial);
    } else if (HMS_4CH::isValidSerial(serial)) {
        i = std::make_shared<HMS_4CH>(_radioCmt.get(), serial);
    } else if (HMS_2CH::isValidSerial(serial)) {
        i = std::make_shared<HMS_2CH>(_radioCmt.get(), serial);
    } else if (HMS_1CH::isValidSerial(serial)) {
        i = std::make_shared<HMS_1CH>(_radioCmt.get(), serial);
    } else if (HMS_1CHv2::isValidSerial(serial)) {
        i = std::make_shared<HMS_1CHv2>(_radioCmt.get(), serial);
    } else if (HM_4CH::isValidSerial(serial)) {
        i = std::make_shared<HM_4CH>(_radioNrf.get(), serial);
    } else if (HM_2CH::isValidSerial(serial)) {
        i = std::make_shared<HM_2CH>(_radioNrf.get(), serial);
    } else if (HM_1CH::isValidSerial(serial)) {
        i = std::make_shared<HM_1CH>(_radioNrf.get(), serial);
    } else if (HERF_1CH::isValidSerial(serial)) {
        i = std::make_shared<HERF_1CH>(_radioNrf.get(), serial);
    } else if (HERF_2CH::isValidSerial(serial)) {
        i = std::make_shared<HERF_2CH>(_radioNrf.get(), serial);
    } else if (HERF_4CH::isValidSerial(serial)) {
        i = std::make_shared<HERF_4CH>(_radioNrf.get(), serial);
    }

    if (i) {
        i->setName(name);
        i->init();
        _inverters.push_back(std::move(i));
        return _inverters.back();
    }

    return nullptr;
}

std::shared_ptr<InverterAbstract> HoymilesClass::getInverterByPos(const uint8_t pos)
{
    if (pos >= _inverters.size()) {
        return nullptr;
    } else {
        return _inverters[pos];
    }
}

std::shared_ptr<InverterAbstract> HoymilesClass::getInverterBySerial(const uint64_t serial)
{
    for (auto& inv : _inverters) {
        if (inv->serial() == serial) {
            return inv;
        }
    }
    return nullptr;
}

std::shared_ptr<InverterAbstract> HoymilesClass::getInverterByFragment(const fragment_t& fragment)
{
    if (fragment.len <= 4) {
        return nullptr;
    }

    for (auto& inv : _inverters) {
        serial_u p;
        p.u64 = inv->serial();

        if ((p.b[3] == fragment.fragment[1])
            && (p.b[2] == fragment.fragment[2])
            && (p.b[1] == fragment.fragment[3])
            && (p.b[0] == fragment.fragment[4])) {

            return inv;
        }
    }
    return nullptr;
}

void HoymilesClass::removeInverterBySerial(const uint64_t serial)
{
    for (uint8_t i = 0; i < _inverters.size(); i++) {
        if (_inverters[i]->serial() == serial) {
            std::lock_guard<std::mutex> lock(_mutex);
            _inverters[i]->getRadio()->removeCommands(_inverters[i].get());
            _inverters.erase(_inverters.begin() + i);
            return;
        }
    }
}

size_t HoymilesClass::getNumInverters() const
{
    return _inverters.size();
}

HoymilesRadio_NRF* HoymilesClass::getRadioNrf()
{
    return _radioNrf.get();
}

HoymilesRadio_CMT* HoymilesClass::getRadioCmt()
{
    return _radioCmt.get();
}

bool HoymilesClass::isAllRadioIdle() const
{
    return _radioNrf.get()->isIdle() && _radioCmt.get()->isIdle();
}

uint32_t HoymilesClass::PollInterval() const
{
    return _pollInterval;
}

void HoymilesClass::setPollInterval(const uint32_t interval)
{
    _pollInterval = interval;
}

void HoymilesClass::setMessageOutput(Print* output)
{
    _messageOutput = output;
}

Print* HoymilesClass::getMessageOutput()
{
    return _messageOutput;
}
