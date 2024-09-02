// SPDX-License-Identifier: GPL-2.0-or-later
#include "PylontechCanReceiver.h"
#include "MessageOutput.h"
#include "PinMapping.h"
#include <driver/twai.h>
#include <ctime>

bool PylontechCanReceiver::init(bool verboseLogging)
{
    return BatteryCanReceiver::init(verboseLogging, "Pylontech");
}


void PylontechCanReceiver::onMessage(twai_message_t rx_message)
{
    switch (rx_message.identifier) {
        case 0x351: {
            _stats->_chargeVoltage = this->scaleValue(this->readUnsignedInt16(rx_message.data), 0.1);
            _stats->_chargeCurrentLimitation = this->scaleValue(this->readSignedInt16(rx_message.data + 2), 0.1);
            _stats->_dischargeCurrentLimitation = this->scaleValue(this->readSignedInt16(rx_message.data + 4), 0.1);

            if (_verboseLogging) {
                MessageOutput.printf("[Pylontech] chargeVoltage: %f chargeCurrentLimitation: %f dischargeCurrentLimitation: %f\r\n",
                        _stats->_chargeVoltage, _stats->_chargeCurrentLimitation, _stats->_dischargeCurrentLimitation);
            }
            break;
        }

        case 0x355: {
            _stats->setSoC(static_cast<uint8_t>(this->readUnsignedInt16(rx_message.data)), 0/*precision*/, millis());
            _stats->_stateOfHealth = this->readUnsignedInt16(rx_message.data + 2);

            if (_verboseLogging) {
                MessageOutput.printf("[Pylontech] soc: %f soh: %d\r\n",
                        _stats->getSoC(), _stats->_stateOfHealth);
            }
            break;
        }

        case 0x356: {
            _stats->setVoltage(this->scaleValue(this->readSignedInt16(rx_message.data), 0.01), millis());
            _stats->setCurrent(this->scaleValue(this->readSignedInt16(rx_message.data + 2), 0.1), 1/*precision*/, millis());
            _stats->_temperature = this->scaleValue(this->readSignedInt16(rx_message.data + 4), 0.1);

            if (_verboseLogging) {
                MessageOutput.printf("[Pylontech] voltage: %f current: %f temperature: %f\r\n",
                        _stats->getVoltage(), _stats->getChargeCurrent(), _stats->_temperature);
            }
            break;
        }

        case 0x359: {
            uint16_t alarmBits = rx_message.data[0];
            _stats->_alarmOverCurrentDischarge = this->getBit(alarmBits, 7);
            _stats->_alarmUnderTemperature = this->getBit(alarmBits, 4);
            _stats->_alarmOverTemperature = this->getBit(alarmBits, 3);
            _stats->_alarmUnderVoltage = this->getBit(alarmBits, 2);
            _stats->_alarmOverVoltage= this->getBit(alarmBits, 1);

            alarmBits = rx_message.data[1];
            _stats->_alarmBmsInternal= this->getBit(alarmBits, 3);
            _stats->_alarmOverCurrentCharge = this->getBit(alarmBits, 0);

            if (_verboseLogging) {
                MessageOutput.printf("[Pylontech] Alarms: %d %d %d %d %d %d %d\r\n",
                        _stats->_alarmOverCurrentDischarge,
                        _stats->_alarmUnderTemperature,
                        _stats->_alarmOverTemperature,
                        _stats->_alarmUnderVoltage,
                        _stats->_alarmOverVoltage,
                        _stats->_alarmBmsInternal,
                        _stats->_alarmOverCurrentCharge);
            }

            uint16_t warningBits = rx_message.data[2];
            _stats->_warningHighCurrentDischarge = this->getBit(warningBits, 7);
            _stats->_warningLowTemperature = this->getBit(warningBits, 4);
            _stats->_warningHighTemperature = this->getBit(warningBits, 3);
            _stats->_warningLowVoltage = this->getBit(warningBits, 2);
            _stats->_warningHighVoltage = this->getBit(warningBits, 1);

            warningBits = rx_message.data[3];
            _stats->_warningBmsInternal= this->getBit(warningBits, 3);
            _stats->_warningHighCurrentCharge = this->getBit(warningBits, 0);

            if (_verboseLogging) {
                MessageOutput.printf("[Pylontech] Warnings: %d %d %d %d %d %d %d\r\n",
                        _stats->_warningHighCurrentDischarge,
                        _stats->_warningLowTemperature,
                        _stats->_warningHighTemperature,
                        _stats->_warningLowVoltage,
                        _stats->_warningHighVoltage,
                        _stats->_warningBmsInternal,
                        _stats->_warningHighCurrentCharge);
            }
            break;
        }

        case 0x35E: {
            String manufacturer(reinterpret_cast<char*>(rx_message.data),
                    rx_message.data_length_code);

            if (manufacturer.isEmpty()) { break; }

            if (_verboseLogging) {
                MessageOutput.printf("[Pylontech] Manufacturer: %s\r\n", manufacturer.c_str());
            }

            _stats->setManufacturer(std::move(manufacturer));
            break;
        }

        case 0x35C: {
            uint16_t chargeStatusBits = rx_message.data[0];
            _stats->_chargeEnabled = this->getBit(chargeStatusBits, 7);
            _stats->_dischargeEnabled = this->getBit(chargeStatusBits, 6);
            _stats->_chargeImmediately = this->getBit(chargeStatusBits, 5);

            if (_verboseLogging) {
                MessageOutput.printf("[Pylontech] chargeStatusBits: %d %d %d\r\n",
                    _stats->_chargeEnabled,
                    _stats->_dischargeEnabled,
                    _stats->_chargeImmediately);
            }

            break;
        }

        default:
            return; // do not update last update timestamp
            break;
    }

    _stats->setLastUpdate(millis());
}

// Currently not called because there is no nice way to integrate it right now
#ifdef PYLONTECH_DUMMY
void PylontechCanReceiver::dummyData()
{
    static uint32_t lastUpdate = millis();
    static uint8_t issues = 0;

    if (millis() < (lastUpdate + 5 * 1000)) { return; }

    lastUpdate = millis();
    _stats->setLastUpdate(lastUpdate);

    auto dummyFloat = [](int offset) -> float {
        return offset + (static_cast<float>((lastUpdate + offset) % 10) / 10);
    };

    _stats->setManufacturer("Pylontech US3000C");
    _stats->setSoC(42, 0/*precision*/, millis());
    _stats->_chargeVoltage = dummyFloat(50);
    _stats->_chargeCurrentLimitation = dummyFloat(33);
    _stats->_dischargeCurrentLimitation = dummyFloat(12);
    _stats->_stateOfHealth = 99;
    _stats->setVoltage(48.67, millis());
    _stats->setCurrent(dummyFloat(-1), 1/*precision*/, millis());
    _stats->_temperature = dummyFloat(20);

    _stats->_chargeEnabled = true;
    _stats->_dischargeEnabled = true;
    _stats->_chargeImmediately = false;

    _stats->_warningHighCurrentDischarge = false;
    _stats->_warningHighCurrentCharge = false;
    _stats->_warningLowTemperature = false;
    _stats->_warningHighTemperature = false;
    _stats->_warningLowVoltage = false;
    _stats->_warningHighVoltage = false;
    _stats->_warningBmsInternal = false;

    _stats->_alarmOverCurrentDischarge = false;
    _stats->_alarmOverCurrentCharge = false;
    _stats->_alarmUnderTemperature = false;
    _stats->_alarmOverTemperature = false;
    _stats->_alarmUnderVoltage = false;
    _stats->_alarmOverVoltage = false;
    _stats->_alarmBmsInternal = false;

    if (issues == 1 || issues == 3) {
        _stats->_warningHighCurrentDischarge = true;
        _stats->_warningHighCurrentCharge = true;
        _stats->_warningLowTemperature = true;
        _stats->_warningHighTemperature = true;
        _stats->_warningLowVoltage = true;
        _stats->_warningHighVoltage = true;
        _stats->_warningBmsInternal = true;
    }

    if (issues == 2 || issues == 3) {
        _stats->_alarmOverCurrentDischarge = true;
        _stats->_alarmOverCurrentCharge = true;
        _stats->_alarmUnderTemperature = true;
        _stats->_alarmOverTemperature = true;
        _stats->_alarmUnderVoltage = true;
        _stats->_alarmOverVoltage = true;
        _stats->_alarmBmsInternal = true;
    }

    if (issues == 4) {
        _stats->_warningHighCurrentCharge = true;
        _stats->_warningLowTemperature = true;
        _stats->_alarmUnderVoltage = true;
        _stats->_dischargeEnabled = false;
        _stats->_chargeImmediately = true;
    }

    issues = (issues + 1) % 5;
}
#endif
