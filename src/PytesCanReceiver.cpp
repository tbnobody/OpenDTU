// SPDX-License-Identifier: GPL-2.0-or-later
#include "PytesCanReceiver.h"
#include "MessageOutput.h"
#include "PinMapping.h"
#include <driver/twai.h>
#include <ctime>

bool PytesCanReceiver::init(bool verboseLogging)
{
    return BatteryCanReceiver::init(verboseLogging, "Pytes");
}

void PytesCanReceiver::onMessage(twai_message_t rx_message)
{
    switch (rx_message.identifier) {
        case 0x351: {
            _stats->_chargeVoltageLimit = this->scaleValue(this->readUnsignedInt16(rx_message.data), 0.1);
            _stats->_chargeCurrentLimit = this->scaleValue(this->readUnsignedInt16(rx_message.data + 2), 0.1);
            _stats->_dischargeCurrentLimit = this->scaleValue(this->readUnsignedInt16(rx_message.data + 4), 0.1);
            _stats->_dischargeVoltageLimit = this->scaleValue(this->readSignedInt16(rx_message.data + 6), 0.1);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] chargeVoltageLimit: %f chargeCurrentLimit: %f dischargeCurrentLimit: %f dischargeVoltageLimit: %f\r\n",
                        _stats->_chargeVoltageLimit, _stats->_chargeCurrentLimit,
                        _stats->_dischargeCurrentLimit, _stats->_dischargeVoltageLimit);
            }
            break;
        }

        case 0x355: {
            _stats->setSoC(static_cast<uint8_t>(this->readUnsignedInt16(rx_message.data)), 0/*precision*/, millis());
            _stats->_stateOfHealth = this->readUnsignedInt16(rx_message.data + 2);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] soc: %d soh: %d\r\n",
                        _stats->getSoC(), _stats->_stateOfHealth);
            }
            break;
        }

        case 0x356: {
            _stats->setVoltage(this->scaleValue(this->readSignedInt16(rx_message.data), 0.01), millis());
            _stats->_current = this->scaleValue(this->readSignedInt16(rx_message.data + 2), 0.1);
            _stats->_temperature = this->scaleValue(this->readSignedInt16(rx_message.data + 4), 0.1);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] voltage: %f current: %f temperature: %f\r\n",
                        _stats->getVoltage(), _stats->_current, _stats->_temperature);
            }
            break;
        }

        case 0x35A: { // Alarms and Warnings
            uint16_t alarmBits = rx_message.data[0];
            _stats->_alarmOverVoltage = this->getBit(alarmBits, 2);
            _stats->_alarmUnderVoltage = this->getBit(alarmBits, 4);
            _stats->_alarmOverTemperature = this->getBit(alarmBits, 6);

            alarmBits = rx_message.data[1];
            _stats->_alarmUnderTemperature = this->getBit(alarmBits, 0);
            _stats->_alarmOverTemperatureCharge = this->getBit(alarmBits, 2);
            _stats->_alarmUnderTemperatureCharge = this->getBit(alarmBits, 4);
            _stats->_alarmOverCurrentDischarge = this->getBit(alarmBits, 6);

            alarmBits = rx_message.data[2];
            _stats->_alarmOverCurrentCharge = this->getBit(alarmBits, 0);
            _stats->_alarmInternalFailure = this->getBit(alarmBits, 6);

            alarmBits = rx_message.data[3];
            _stats->_alarmCellImbalance = this->getBit(alarmBits, 0);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] Alarms: %d %d %d %d %d %d %d %d %d %d\r\n",
                        _stats->_alarmOverVoltage,
                        _stats->_alarmUnderVoltage,
                        _stats->_alarmOverTemperature,
                        _stats->_alarmUnderTemperature,
                        _stats->_alarmOverTemperatureCharge,
                        _stats->_alarmUnderTemperatureCharge,
                        _stats->_alarmOverCurrentDischarge,
                        _stats->_alarmOverCurrentCharge,
                        _stats->_alarmInternalFailure,
                        _stats->_alarmCellImbalance);
            }

            uint16_t warningBits = rx_message.data[4];
            _stats->_warningHighVoltage = this->getBit(warningBits, 2);
            _stats->_warningLowVoltage = this->getBit(warningBits, 4);
            _stats->_warningHighTemperature = this->getBit(warningBits, 6);

            warningBits = rx_message.data[5];
            _stats->_warningLowTemperature = this->getBit(warningBits, 0);
            _stats->_warningHighTemperatureCharge = this->getBit(warningBits, 2);
            _stats->_warningLowTemperatureCharge = this->getBit(warningBits, 4);
            _stats->_warningHighDischargeCurrent = this->getBit(warningBits, 6);

            warningBits = rx_message.data[6];
            _stats->_warningHighChargeCurrent = this->getBit(warningBits, 0);
            _stats->_warningInternalFailure = this->getBit(warningBits, 6);

            warningBits = rx_message.data[7];
            _stats->_warningCellImbalance = this->getBit(warningBits, 0);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] Warnings: %d %d %d %d %d %d %d %d %d %d\r\n",
                        _stats->_warningHighVoltage,
                        _stats->_warningLowVoltage,
                        _stats->_warningHighTemperature,
                        _stats->_warningLowTemperature,
                        _stats->_warningHighTemperatureCharge,
                        _stats->_warningLowTemperatureCharge,
                        _stats->_warningHighDischargeCurrent,
                        _stats->_warningHighChargeCurrent,
                        _stats->_warningInternalFailure,
                        _stats->_warningCellImbalance);
            }
            break;
        }

        case 0x35E: {
            String manufacturer(reinterpret_cast<char*>(rx_message.data),
                    rx_message.data_length_code);

            if (manufacturer.isEmpty()) { break; }

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] Manufacturer: %s\r\n", manufacturer.c_str());
            }

            _stats->setManufacturer(std::move(manufacturer));
            break;
        }

        case 0x35F: { // BatteryInfo
            auto fwVersionPart1 = String(this->readUnsignedInt8(rx_message.data + 2));
            auto fwVersionPart2 = String(this->readUnsignedInt8(rx_message.data + 3));
            _stats->_fwversion = "v" + fwVersionPart1 + "." + fwVersionPart2;

            _stats->_availableCapacity = this->readUnsignedInt16(rx_message.data + 4);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] fwversion: %s availableCapacity: %d Ah\r\n",
                        _stats->_fwversion.c_str(), _stats->_availableCapacity);
            }
            break;
        }

        case 0x372: { // BankInfo
            _stats->_moduleCountOnline = this->readUnsignedInt16(rx_message.data);
            _stats->_moduleCountBlockingCharge = this->readUnsignedInt16(rx_message.data + 2);
            _stats->_moduleCountBlockingDischarge = this->readUnsignedInt16(rx_message.data + 4);
            _stats->_moduleCountOffline = this->readUnsignedInt16(rx_message.data + 6);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] moduleCountOnline: %d moduleCountBlockingCharge: %d moduleCountBlockingDischarge: %d moduleCountOffline: %d\r\n",
                        _stats->_moduleCountOnline, _stats->_moduleCountBlockingCharge,
                        _stats->_moduleCountBlockingDischarge, _stats->_moduleCountOffline);
            }
            break;
        }

        case 0x373: { // CellInfo
            _stats->_cellMinMilliVolt = this->readUnsignedInt16(rx_message.data);
            _stats->_cellMaxMilliVolt = this->readUnsignedInt16(rx_message.data + 2);
            _stats->_cellMinTemperature = this->readUnsignedInt16(rx_message.data + 4) - 273;
            _stats->_cellMaxTemperature = this->readUnsignedInt16(rx_message.data + 6) - 273;

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] lowestCellMilliVolt: %d highestCellMilliVolt: %d minimumCellTemperature: %f maximumCellTemperature: %f\r\n",
                        _stats->_cellMinMilliVolt, _stats->_cellMaxMilliVolt,
                        _stats->_cellMinTemperature, _stats->_cellMaxTemperature);
            }
            break;
        }

        case 0x374: { // Battery/Cell name (string) with "Lowest Cell Voltage"
            String cellMinVoltageName(reinterpret_cast<char*>(rx_message.data),
                    rx_message.data_length_code);

            if (cellMinVoltageName.isEmpty()) { break; }

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] cellMinVoltageName: %s\r\n",
                        cellMinVoltageName.c_str());
            }

            _stats->_cellMinVoltageName = cellMinVoltageName;
            break;
        }

        case 0x375: { // Battery/Cell name (string) with "Highest Cell Voltage"
            String cellMaxVoltageName(reinterpret_cast<char*>(rx_message.data),
                    rx_message.data_length_code);

            if (cellMaxVoltageName.isEmpty()) { break; }

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] cellMaxVoltageName: %s\r\n",
                        cellMaxVoltageName.c_str());
            }

            _stats->_cellMaxVoltageName = cellMaxVoltageName;
            break;
        }

        case 0x376: { // Battery/Cell name (string) with "Minimum Cell Temperature"
            String cellMinTemperatureName(reinterpret_cast<char*>(rx_message.data),
                    rx_message.data_length_code);

            if (cellMinTemperatureName.isEmpty()) { break; }

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] cellMinTemperatureName: %s\r\n",
                        cellMinTemperatureName.c_str());
            }

            _stats->_cellMinTemperatureName = cellMinTemperatureName;
            break;
        }

        case 0x377: { // Battery/Cell name (string) with "Maximum Cell Temperature"
            String cellMaxTemperatureName(reinterpret_cast<char*>(rx_message.data),
                    rx_message.data_length_code);

            if (cellMaxTemperatureName.isEmpty()) { break; }

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] cellMaxTemperatureName: %s\r\n",
                        cellMaxTemperatureName.c_str());
            }

            _stats->_cellMaxTemperatureName = cellMaxTemperatureName;
            break;
        }

        case 0x378: { // History: Charged / Discharged Energy
            _stats->_chargedEnergy = this->scaleValue(this->readUnsignedInt32(rx_message.data), 0.1);
            _stats->_dischargedEnergy = this->scaleValue(this->readUnsignedInt32(rx_message.data + 4), 0.1);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] chargedEnergy: %f dischargedEnergy: %f\r\n",
                        _stats->_chargedEnergy, _stats->_dischargedEnergy);
            }
            break;
        }

        case 0x379: { // BatterySize: Installed Ah
            _stats->_totalCapacity = this->readUnsignedInt16(rx_message.data);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] totalCapacity: %d Ah\r\n",
                        _stats->_totalCapacity);
            }
            break;
        }

        case 0x380: { // Serialnumber - part 1
            String snPart1(reinterpret_cast<char*>(rx_message.data),
                    rx_message.data_length_code);

            if (snPart1.isEmpty() || !isgraph(snPart1.charAt(0))) { break; }

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] snPart1: %s\r\n", snPart1.c_str());
            }

            _stats->_serialPart1 = snPart1;
            _stats->updateSerial();
            break;
        }

        case 0x381: {  // Serialnumber - part 2
            String snPart2(reinterpret_cast<char*>(rx_message.data),
                    rx_message.data_length_code);

            if (snPart2.isEmpty() || !isgraph(snPart2.charAt(0))) { break; }

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] snPart2: %s\r\n", snPart2.c_str());
            }

            _stats->_serialPart2 = snPart2;
            _stats->updateSerial();
            break;
        }

        default:
            return; // do not update last update timestamp
            break;
    }

    _stats->setLastUpdate(millis());
}
