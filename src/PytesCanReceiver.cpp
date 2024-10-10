// SPDX-License-Identifier: GPL-2.0-or-later
#include "PytesCanReceiver.h"
#include "MessageOutput.h"
#include "PinMapping.h"
#include <driver/twai.h>
#include <ctime>

namespace {

static void pytesSetCellLabel(String& label, uint16_t value) {
    char name[8];
    snprintf(name, sizeof(name), "%02d%02d", value & 0xff, value >> 8);
    label = name;  // updates existing string in-place
}

static uint32_t popCount(uint32_t val) {
    uint32_t cnt = 0;
    for (; val; ++cnt)
        val &= val - 1;
    return cnt;
}

};  // namespace

bool PytesCanReceiver::init(bool verboseLogging)
{
    return BatteryCanReceiver::init(verboseLogging, "Pytes");
}

void PytesCanReceiver::onMessage(twai_message_t rx_message)
{
    switch (rx_message.identifier) {
        case 0x351:
        case 0x400: {
            _stats->_chargeVoltageLimit = this->scaleValue(this->readUnsignedInt16(rx_message.data), 0.1);
            _stats->_chargeCurrentLimit = this->scaleValue(this->readUnsignedInt16(rx_message.data + 2), 0.1);
            _stats->setDischargeCurrentLimit(this->scaleValue(this->readUnsignedInt16(rx_message.data + 4), 0.1), millis());
            _stats->_dischargeVoltageLimit = this->scaleValue(this->readSignedInt16(rx_message.data + 6), 0.1);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] chargeVoltageLimit: %f chargeCurrentLimit: %f dischargeCurrentLimit: %f dischargeVoltageLimit: %f\r\n",
                        _stats->_chargeVoltageLimit, _stats->_chargeCurrentLimit,
                        _stats->getDischargeCurrentLimit(), _stats->_dischargeVoltageLimit);
            }
            break;
        }

        case 0x355: {  // Victron protocol: SOC/SOH
            _stats->setSoC(static_cast<uint8_t>(this->readUnsignedInt16(rx_message.data)), 0/*precision*/, millis());
            _stats->_stateOfHealth = this->readUnsignedInt16(rx_message.data + 2);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] soc: %f soh: %d\r\n",
                        _stats->getSoC(), _stats->_stateOfHealth);
            }
            break;
        }

        case 0x356:
        case 0x405: {
            _stats->setVoltage(this->scaleValue(this->readSignedInt16(rx_message.data), 0.01), millis());
            _stats->setCurrent(this->scaleValue(this->readSignedInt16(rx_message.data + 2), 0.1), 1/*precision*/, millis());
            _stats->_temperature = this->scaleValue(this->readSignedInt16(rx_message.data + 4), 0.1);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] voltage: %f current: %f temperature: %f\r\n",
                        _stats->getVoltage(), _stats->getChargeCurrent(), _stats->_temperature);
            }
            break;
        }

        case 0x35A: { // Victron protocol: Alarms and Warnings
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

        case 0x35E:
        case 0x40A: {
            String manufacturer(reinterpret_cast<char*>(rx_message.data),
                    rx_message.data_length_code);

            if (manufacturer.isEmpty()) { break; }

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] Manufacturer: %s\r\n", manufacturer.c_str());
            }

            _stats->setManufacturer(manufacturer);
            break;
        }

        case 0x35F: { // Victron protocol: BatteryInfo
            auto fwVersionPart1 = String(this->readUnsignedInt8(rx_message.data + 2));
            auto fwVersionPart2 = String(this->readUnsignedInt8(rx_message.data + 3));
            _stats->_fwversion = "v" + fwVersionPart1 + "." + fwVersionPart2;

            _stats->_availableCapacity = this->readUnsignedInt16(rx_message.data + 4);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] fwversion: %s availableCapacity: %f Ah\r\n",
                        _stats->_fwversion.c_str(), _stats->_availableCapacity);
            }
            break;
        }

        case 0x360: { // Victron protocol: Charging request
            _stats->_chargeImmediately = rx_message.data[0]; // 0xff requests charging.
            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] chargeImmediately: %d\r\n",
                        _stats->_chargeImmediately);
            }
            break;
        }

        case 0x372: { // Victron protocol: BankInfo
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

        case 0x373: { // Victron protocol: CellInfo
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

        case 0x374: { // Victron protocol: Battery/Cell name (string) with "Lowest Cell Voltage"
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

        case 0x375: { // Victron protocol: Battery/Cell name (string) with "Highest Cell Voltage"
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

        case 0x376: { // Victron Protocol: Battery/Cell name (string) with "Minimum Cell Temperature"
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

        case 0x377: { // Victron Protocol: Battery/Cell name (string) with "Maximum Cell Temperature"
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

        case 0x378:
        case 0x41e: { // History: Charged / Discharged Energy
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
                MessageOutput.printf("[Pytes] totalCapacity: %f Ah\r\n",
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

        case 0x401: { // Pytes protocol: Highest/Lowest Cell Voltage
            _stats->_cellMaxMilliVolt = this->readUnsignedInt16(rx_message.data);
            _stats->_cellMinMilliVolt = this->readUnsignedInt16(rx_message.data + 2);
            pytesSetCellLabel(_stats->_cellMaxVoltageName, this->readUnsignedInt8(rx_message.data + 4));
            pytesSetCellLabel(_stats->_cellMinVoltageName, this->readUnsignedInt8(rx_message.data + 6));

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] lowestCellMilliVolt: %d highestCellMilliVolt: %d cellMinVoltageName: %s cellMaxVoltageName: %s\r\n",
                        _stats->_cellMinMilliVolt, _stats->_cellMaxMilliVolt,
                        _stats->_cellMinVoltageName.c_str(), _stats->_cellMaxVoltageName.c_str());
            }
            break;
        }

        case 0x402: { // Pytes protocol: Highest/Lowest Cell Temperature
            _stats->_cellMaxTemperature = this->scaleValue(this->readUnsignedInt16(rx_message.data), 0.1);
            _stats->_cellMinTemperature = this->scaleValue(this->readUnsignedInt16(rx_message.data + 2), 0.1);
            pytesSetCellLabel(_stats->_cellMaxTemperatureName, this->readUnsignedInt16(rx_message.data + 4));
            pytesSetCellLabel(_stats->_cellMinTemperatureName, this->readUnsignedInt16(rx_message.data + 6));

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] minimumCellTemperature: %f maximumCellTemperature: %f cellMinTemperatureName: %s cellMaxTemperatureName: %s\r\n",
                        _stats->_cellMinTemperature, _stats->_cellMaxTemperature,
                        _stats->_cellMinTemperatureName.c_str(), _stats->_cellMaxTemperatureName.c_str());
            }
            break;
        }

        case 0x403: { // Pytes protocol: Alarms and Warnings (part 1)
            uint32_t alarmBits1 = this->readUnsignedInt32(rx_message.data);
            uint32_t alarmBits2 = this->readUnsignedInt32(rx_message.data + 4);
            uint32_t mergedBits = alarmBits1 | alarmBits2;

            bool overVoltage = this->getBit(mergedBits, 0);
            bool highVoltage = this->getBit(mergedBits, 1);
            bool lowVoltage = this->getBit(mergedBits, 3);
            bool underVoltage = this->getBit(mergedBits, 4);
            bool overTemp = this->getBit(mergedBits, 8);
            bool highTemp = this->getBit(mergedBits, 9);
            bool lowTemp = this->getBit(mergedBits, 11);
            bool underTemp = this->getBit(mergedBits, 12);
            bool overCurrentDischarge = this->getBit(mergedBits, 17) || this->getBit(mergedBits, 18);
            bool overCurrentCharge = this->getBit(mergedBits, 19) || this->getBit(mergedBits, 20);
            bool highCurrentDischarge = this->getBit(mergedBits, 21);
            bool highCurrentCharge = this->getBit(mergedBits, 22);
            bool stateCharging = this->getBit(mergedBits, 26);
            bool stateDischarging = this->getBit(mergedBits, 27);

            _stats->_alarmOverVoltage = overVoltage;
            _stats->_alarmUnderVoltage = underVoltage;
            _stats->_alarmOverTemperature = stateDischarging && overTemp;
            _stats->_alarmUnderTemperature = stateDischarging && underTemp;
            _stats->_alarmOverTemperatureCharge = stateCharging && overTemp;
            _stats->_alarmUnderTemperatureCharge = stateCharging && underTemp;

            _stats->_alarmOverCurrentDischarge = overCurrentDischarge;
            _stats->_alarmOverCurrentCharge = overCurrentCharge;

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] Alarms: %d %d %d %d %d %d %d %d\r\n",
                        _stats->_alarmOverVoltage,
                        _stats->_alarmUnderVoltage,
                        _stats->_alarmOverTemperature,
                        _stats->_alarmUnderTemperature,
                        _stats->_alarmOverTemperatureCharge,
                        _stats->_alarmUnderTemperatureCharge,
                        _stats->_alarmOverCurrentDischarge,
                        _stats->_alarmOverCurrentCharge);
            }

            _stats->_warningHighVoltage = highVoltage;
            _stats->_warningLowVoltage = lowVoltage;
            _stats->_warningHighTemperature = stateDischarging && highTemp;
            _stats->_warningLowTemperature = stateDischarging && lowTemp;
            _stats->_warningHighTemperatureCharge = stateCharging && highTemp;
            _stats->_warningLowTemperatureCharge = stateCharging && lowTemp;

            _stats->_warningHighDischargeCurrent = highCurrentDischarge;
            _stats->_warningHighChargeCurrent = highCurrentCharge;

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] Warnings: %d %d %d %d %d %d %d %d\r\n",
                        _stats->_warningHighVoltage,
                        _stats->_warningLowVoltage,
                        _stats->_warningHighTemperature,
                        _stats->_warningLowTemperature,
                        _stats->_warningHighTemperatureCharge,
                        _stats->_warningLowTemperatureCharge,
                        _stats->_warningHighDischargeCurrent,
                        _stats->_warningHighChargeCurrent);
            }
            break;
        }

        case 0x404: { // Pytes protocol: SOC/SOH
            // soc (byte 0+1) isn't used here since it is generated with higher
            // precision in message 0x0409 below.
            _stats->_stateOfHealth = this->readUnsignedInt16(rx_message.data + 2);
            _stats->_chargeCycles = this->readUnsignedInt16(rx_message.data + 6);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] soh: %d cycles: %d\r\n",
                        _stats->_stateOfHealth, _stats->_chargeCycles);
            }
            break;
        }

        case 0x406: { // Pytes protocol: alarms (part 2)
            uint32_t alarmBits = this->readUnsignedInt32(rx_message.data);
            _stats->_alarmInternalFailure = this->getBit(alarmBits, 15);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] internalFailure: %d (bits: %08x)\r\n",
                        _stats->_alarmInternalFailure, alarmBits);
            }
            break;
        }

        case 0x408: { // Pytes protocol: charge status
            bool chargeEnabled = rx_message.data[0];
            bool dischargeEnabled = rx_message.data[1];
            _stats->_chargeImmediately = rx_message.data[2];
            // Note: Should use std::popcount once supported by the compiler.
            _stats->_moduleCountBlockingCharge = popCount(rx_message.data[5]);
            _stats->_moduleCountBlockingDischarge = popCount(rx_message.data[6]);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] chargeEnabled: %d dischargeEnabled: %d chargeImmediately: %d moduleCountBlockingDischarge: %d moduleCountBlockingCharge: %d\r\n",
                    chargeEnabled, dischargeEnabled, _stats->_chargeImmediately,
                    _stats->_moduleCountBlockingCharge, _stats->_moduleCountBlockingDischarge);
            }
            break;
        }

        case 0x409: { // Pytes protocol: full mAh / remaining mAh
            _stats->_totalCapacity = this->scaleValue(this->readUnsignedInt32(rx_message.data), 0.001);
            _stats->_availableCapacity = this->scaleValue(this->readUnsignedInt32(rx_message.data + 4), 0.001);
            _stats->_capacityPrecision = 2;
            float soc = 100.0 * _stats->_availableCapacity / _stats->_totalCapacity;
            _stats->setSoC(soc, 2/*precision*/, millis());

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] soc: %.2f totalCapacity: %.2f Ah availableCapacity: %.2f Ah \r\n",
                        soc, _stats->_totalCapacity, _stats->_availableCapacity);
            }
            break;
        }

        case 0x40b: { // Pytes protocol: online / offline module count
            _stats->_moduleCountOnline = this->readUnsignedInt8(rx_message.data + 6);
            _stats->_moduleCountOffline = this->readUnsignedInt8(rx_message.data + 7);

            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] moduleCountOnline: %d moduleCountOffline: %d\r\n",
                        _stats->_moduleCountOnline, _stats->_moduleCountOffline);
            }
            break;
        }

        case 0x40d: { // Pytes protocol: balancing info
            // We don't know the exact unit for this yet, so we only use
            // it to publish active / not active.
            // It is somewhat likely that this is a percentage value on
            // the scale of 0-32768, but that is just a theory.
            _stats->_balance = this->readUnsignedInt16(rx_message.data + 4);
            if (_verboseLogging) {
                MessageOutput.printf("[Pytes] balance: %d\r\n",
                        _stats->_balance);
            }
            break;
        }

        default:
            return; // do not update last update timestamp
            break;
    }

    _stats->setLastUpdate(millis());
}
