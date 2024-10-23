// SPDX-License-Identifier: GPL-2.0-or-later
#include "SBSCanReceiver.h"
#include "MessageOutput.h"
#include "PinMapping.h"
#include <driver/twai.h>
#include <ctime>

bool SBSCanReceiver::init(bool verboseLogging)
{
    _stats->_chargeVoltage =58.4;
    return BatteryCanReceiver::init(verboseLogging, "SBS");
}


void SBSCanReceiver::onMessage(twai_message_t rx_message)
{
    switch (rx_message.identifier) {
        case 0x610: {
            _stats->setVoltage(this->readUnsignedInt16(rx_message.data)* 0.001, millis());
            _stats->_current =(this->readSignedInt16(rx_message.data + 3)) * 0.001;
            _stats->setSoC(static_cast<float>(this->readUnsignedInt16(rx_message.data + 6)), 1, millis());

            if (_verboseLogging) {
                MessageOutput.printf("[SBS Unipower] 1552 SoC: %f Voltage: %f Current: %f\r\n", _stats->getSoC(), _stats->getVoltage(), _stats->_current);
            }
            break;
        }

         case 0x630: {
            int clusterstate = rx_message.data[0];
            switch (clusterstate) {
                case 0:
                    // Battery inactive
                    _stats->_dischargeEnabled = 0;
                    _stats->_chargeEnabled = 0;
                    break;

                case 1:
                    // Battery Discharge mode (recuperation enabled)
                    _stats->_chargeEnabled = 1;
                    _stats->_dischargeEnabled = 1;
                    break;

                case 2:
                    // Battery in charge Mode (discharge with half current possible (45A))
                    _stats->_chargeEnabled = 1;
                    _stats->_dischargeEnabled = 1;
                    break;

                case 4:
                    // Battery Fault
                    _stats->_chargeEnabled = 0;
                    _stats->_dischargeEnabled = 0;
                    break;

                case 8:
                    // Battery Deepsleep
                    _stats->_chargeEnabled = 0;
                    _stats->_dischargeEnabled = 0;
                    break;

                default:
                    _stats->_dischargeEnabled = 0;
                    _stats->_chargeEnabled = 0;
                    break;
            }
            _stats->setManufacturer("SBS UniPower ");

            if (_verboseLogging) {
                MessageOutput.printf("[SBS Unipower] 1584 chargeStatusBits: %d %d\r\n", _stats->_chargeEnabled, _stats->_dischargeEnabled);
            }
            break;
        }

        case 0x640: {
            _stats->_chargeCurrentLimitation = (this->readSignedInt24(rx_message.data + 3) * 0.001);
            _stats->setDischargeCurrentLimit(this->readSignedInt24(rx_message.data) * 0.001, millis());

            if (_verboseLogging) {
                MessageOutput.printf("[SBS Unipower] 1600 Currents  %f, %f \r\n", _stats->_chargeCurrentLimitation, _stats->getDischargeCurrentLimit());
            }
            break;
        }

        case 0x650: {
            byte temp = rx_message.data[0];
            _stats->_temperature = (static_cast<float>(temp)-32) /1.8;

            if (_verboseLogging) {
                MessageOutput.printf("[SBS Unipower] 1616 Temp %f \r\n",_stats->_temperature);
            }
            break;
        }

        case 0x660: {
            uint16_t alarmBits = rx_message.data[0];
            _stats->_alarmUnderTemperature = this->getBit(alarmBits, 1);
            _stats->_alarmOverTemperature = this->getBit(alarmBits, 0);
            _stats->_alarmUnderVoltage = this->getBit(alarmBits, 3);
            _stats->_alarmOverVoltage= this->getBit(alarmBits, 2);
            _stats->_alarmBmsInternal= this->getBit(rx_message.data[1], 2);

            if (_verboseLogging) {
                MessageOutput.printf("[SBS Unipower] 1632 Alarms: %d %d %d %d \r\n ", _stats->_alarmUnderTemperature, _stats->_alarmOverTemperature, _stats->_alarmUnderVoltage,  _stats->_alarmOverVoltage);
            }
            break;
        }

        case 0x670: {
            uint16_t warningBits = rx_message.data[1];
            _stats->_warningHighCurrentDischarge = this->getBit(warningBits, 1);
            _stats->_warningHighCurrentCharge = this->getBit(warningBits, 0);

             if (_verboseLogging) {
                MessageOutput.printf("[SBS Unipower] 1648 Warnings: %d %d \r\n", _stats->_warningHighCurrentDischarge, _stats->_warningHighCurrentCharge);
            }
            break;
        }

        default:
            return; // do not update last update timestamp
            break;
    }

    _stats->setLastUpdate(millis());
}

#ifdef SBSCanReceiver_DUMMY
void SBSCanReceiver::dummyData()
{
    static uint32_t lastUpdate = millis();
    static uint8_t issues = 0;

    if (millis() < (lastUpdate + 5 * 1000)) { return; }

    lastUpdate = millis();
    _stats->setLastUpdate(lastUpdate);

    auto dummyFloat = [](int offset) -> float {
        return offset + (static_cast<float>((lastUpdate + offset) % 10) / 10);
    };

    _stats->setManufacturer("SBS Unipower XL");
    _stats->setSoC(42, 0/*precision*/, millis());
    _stats->_chargeVoltage = dummyFloat(50);
    _stats->_chargeCurrentLimitation = dummyFloat(33);
    _stats->setDischargeCurrentLimit(dummyFloat(12), millis());
    _stats->_stateOfHealth = 99;
    _stats->setVoltage(48.67, millis());
    _stats->_current = dummyFloat(-1);
    _stats->_temperature = dummyFloat(20);

    _stats->_chargeEnabled = true;
    _stats->_dischargeEnabled = true;

    _stats->_warningHighCurrentDischarge = false;
    _stats->_warningHighCurrentCharge = false;

    _stats->_alarmOverCurrentDischarge = false;
    _stats->_alarmOverCurrentCharge = false;
    _stats->_alarmUnderVoltage = false;
    _stats->_alarmOverVoltage = false;


    if (issues == 1 || issues == 3) {
        _stats->_warningHighCurrentDischarge = true;
        _stats->_warningHighCurrentCharge = true;
    }

    if (issues == 2 || issues == 3) {
        _stats->_alarmOverCurrentDischarge = true;
        _stats->_alarmOverCurrentCharge = true;
        _stats->_alarmUnderVoltage = true;
        _stats->_alarmOverVoltage = true;
    }

    if (issues == 4) {
        _stats->_warningHighCurrentCharge = true;
        _stats->_alarmUnderVoltage = true;
        _stats->_dischargeEnabled = false;
    }

    issues = (issues + 1) % 5;
}
#endif
