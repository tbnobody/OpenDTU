// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "Hoymiles.h"
#include "Utils.h"
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

void HoymilesClass::initNRF(SPIClass* initialisedSpiBus, uint8_t pinCE, uint8_t pinIRQ)
{
    _radioNrf->init(initialisedSpiBus, pinCE, pinIRQ);
}

void HoymilesClass::initCMT(int8_t pin_sdio, int8_t pin_clk, int8_t pin_cs, int8_t pin_fcs, int8_t pin_gpio2, int8_t pin_gpio3)
{
    _radioCmt->init(pin_sdio, pin_clk, pin_cs, pin_fcs, pin_gpio2, pin_gpio3);
}

void HoymilesClass::loop()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _radioNrf->loop();
    _radioCmt->loop();

    if (getNumInverters() > 0) {
        if (millis() - _lastPoll > (_pollInterval * 1000)) {
            static uint8_t inverterPos = 0;

            std::shared_ptr<InverterAbstract> iv = getInverterByPos(inverterPos);
            if ((iv == nullptr) || ((iv != nullptr) && (!iv->getRadio()->isInitialized()))) {
                if (++inverterPos >= getNumInverters()) {
                    inverterPos = 0;
                }
            }

            if (iv != nullptr && iv->getRadio()->isInitialized() && iv->getRadio()->isQueueEmpty()) {

                if (iv->getZeroValuesIfUnreachable() && !iv->isReachable()) {
                    Hoymiles.getMessageOutput()->println("Set runtime data to zero");
                    iv->Statistics()->zeroRuntimeData();
                }

                if (iv->getEnablePolling() || iv->getEnableCommands()) {
                    _messageOutput->print("Fetch inverter: ");
                    _messageOutput->println(iv->serial(), HEX);

                    if (!iv->isReachable()) {
                        iv->sendChangeChannelRequest();
                    }

                    iv->sendStatsRequest();

                    // Fetch event log
                    bool force = iv->EventLog()->getLastAlarmRequestSuccess() == CMD_NOK;
                    iv->sendAlarmLogRequest(force);

                    // Fetch limit
                    if (((millis() - iv->SystemConfigPara()->getLastUpdateRequest() > HOY_SYSTEM_CONFIG_PARA_POLL_INTERVAL)
                            && (millis() - iv->SystemConfigPara()->getLastUpdateCommand() > HOY_SYSTEM_CONFIG_PARA_POLL_MIN_DURATION))) {
                        _messageOutput->println("Request SystemConfigPara");
                        iv->sendSystemConfigParaRequest();
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

                    // Fetch dev info (but first fetch stats)
                    if (iv->Statistics()->getLastUpdate() > 0) {
                        bool invalidDevInfo = !iv->DevInfo()->containsValidData()
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

                    // Fetch grid profile
                    if (iv->Statistics()->getLastUpdate() > 0 && iv->GridProfile()->getLastUpdate() == 0) {
                        iv->sendGridOnProFileParaRequest();
                    }

                    _lastPoll = millis();
                }

                if (++inverterPos >= getNumInverters()) {
                    inverterPos = 0;
                }
            }

            // Perform housekeeping of all inverters on day change
            int8_t currentWeekDay = Utils::getWeekDay();
            static int8_t lastWeekDay = -1;
            if (lastWeekDay == -1) {
                lastWeekDay = currentWeekDay;
            } else {
                if (currentWeekDay != lastWeekDay) {

                    for (auto& inv : _inverters) {
                        if (inv->getZeroYieldDayOnMidnight()) {
                            inv->Statistics()->zeroDailyData();
                        }
                    }

                    lastWeekDay = currentWeekDay;
                }
            }
        }
    }
}

std::shared_ptr<InverterAbstract> HoymilesClass::addInverter(const char* name, uint64_t serial)
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
            std::lock_guard<std::mutex> lock(_mutex);
            _inverters.erase(_inverters.begin() + i);
            return;
        }
    }
}

size_t HoymilesClass::getNumInverters()
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

bool HoymilesClass::isAllRadioIdle()
{
    return _radioNrf.get()->isIdle() && _radioCmt.get()->isIdle();
}

uint32_t HoymilesClass::PollInterval()
{
    return _pollInterval;
}

void HoymilesClass::setPollInterval(uint32_t interval)
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