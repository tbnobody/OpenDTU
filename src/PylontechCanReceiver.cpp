#include "PylontechCanReceiver.h"
#include "Battery.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include <CAN.h>
#include <ctime>

//#define PYLONTECH_DEBUG_ENABLED

PylontechCanReceiverClass PylontechCanReceiver;

void PylontechCanReceiverClass::init(int8_t rx, int8_t tx)
{
    CAN.setPins(rx, tx);

    CONFIG_T& config = Configuration.get();

    if (!config.Battery_Enabled) {
        return;
    }

    enable();
}

void PylontechCanReceiverClass::enable()
{
    if (!CAN.begin(500E3)) {
        Hoymiles.getMessageOutput()->println("Starting CAN failed!");
    }
}

void PylontechCanReceiverClass::loop()
{
    CONFIG_T& config = Configuration.get();

    if (!config.Battery_Enabled) {
        return;
    }

    parseCanPackets();
    mqtt();
}

void PylontechCanReceiverClass::mqtt()
{
    CONFIG_T& config = Configuration.get();

    if (!MqttSettings.getConnected()
            || (millis() - _lastPublish) < (config.Mqtt_PublishInterval * 1000)) {
        return;
    }    

    _lastPublish = millis();

    String topic = "battery";
    MqttSettings.publish(topic + "/settings/chargeVoltage", String(Battery.chargeVoltage));
    MqttSettings.publish(topic + "/settings/chargeCurrentLimitation", String(Battery.chargeCurrentLimitation));
    MqttSettings.publish(topic + "/settings/dischargeCurrentLimitation", String(Battery.dischargeCurrentLimitation));
    MqttSettings.publish(topic + "/stateOfCharge", String(Battery.stateOfCharge));
    MqttSettings.publish(topic + "/stateOfHealth", String(Battery.stateOfHealth));
    MqttSettings.publish(topic + "/voltage", String(Battery.voltage));
    MqttSettings.publish(topic + "/current", String(Battery.current));
    MqttSettings.publish(topic + "/temperature", String(Battery.temperature));
    MqttSettings.publish(topic + "/alarm/overCurrentDischarge", String(Battery.alarmOverCurrentDischarge));
    MqttSettings.publish(topic + "/alarm/underTemperature", String(Battery.alarmUnderTemperature));
    MqttSettings.publish(topic + "/alarm/overTemperature", String(Battery.alarmOverTemperature));
    MqttSettings.publish(topic + "/alarm/underVoltage", String(Battery.alarmUnderVoltage));
    MqttSettings.publish(topic + "/alarm/overVoltage", String(Battery.alarmOverVoltage));
    MqttSettings.publish(topic + "/alarm/bmsInternal", String(Battery.alarmBmsInternal));
    MqttSettings.publish(topic + "/alarm/overCurrentCharge", String(Battery.alarmOverCurrentCharge));
    MqttSettings.publish(topic + "/warning/highCurrentDischarge", String(Battery.warningHighCurrentDischarge));
    MqttSettings.publish(topic + "/warning/lowTemperature", String(Battery.warningLowTemperature));
    MqttSettings.publish(topic + "/warning/highTemperature", String(Battery.warningHighTemperature));
    MqttSettings.publish(topic + "/warning/lowVoltage", String(Battery.warningLowVoltage));
    MqttSettings.publish(topic + "/warning/highVoltage", String(Battery.warningHighVoltage));
    MqttSettings.publish(topic + "/warning/bmsInternal", String(Battery.warningBmsInternal));
    MqttSettings.publish(topic + "/warning/highCurrentCharge", String(Battery.warningHighCurrentCharge));
    MqttSettings.publish(topic + "/manufacturer", Battery.manufacturer);
    MqttSettings.publish(topic + "/charging/chargeEnabled", String(Battery.chargeEnabled));
    MqttSettings.publish(topic + "/charging/dischargeEnabled", String(Battery.dischargeEnabled));
    MqttSettings.publish(topic + "/charging/chargeImmediately", String(Battery.chargeImmediately));
}

void PylontechCanReceiverClass::parseCanPackets()
{
    // try to parse packet
    int packetSize = CAN.parsePacket();

    if ((packetSize <= 0 && CAN.packetId() == -1)
            || CAN.packetRtr()) {
        return;
    }

    switch (CAN.packetId()) {
        case 0x351: {
            Battery.chargeVoltage = this->scaleValue(this->readUnsignedInt16(), 0.1);
            Battery.chargeCurrentLimitation = this->scaleValue(this->readSignedInt16(), 0.1);
            Battery.dischargeCurrentLimitation = this->scaleValue(this->readSignedInt16(), 0.1);

#ifdef PYLONTECH_DEBUG_ENABLED
            Hoymiles.getMessageOutput()->printf("[Pylontech] chargeVoltage: %f chargeCurrentLimitation: %f dischargeCurrentLimitation: %f\n",
                Battery.chargeVoltage, Battery.chargeCurrentLimitation, Battery.dischargeCurrentLimitation);
#endif
            break;
        }

        case 0x355: {
            Battery.stateOfCharge = this->readUnsignedInt16();
            Battery.stateOfChargeLastUpdate = millis();
            Battery.stateOfHealth = this->readUnsignedInt16();

#ifdef PYLONTECH_DEBUG_ENABLED
            Hoymiles.getMessageOutput()->printf("[Pylontech] soc: %d soh: %d\n",
                Battery.stateOfCharge, Battery.stateOfHealth);
#endif
            break;
        }

        case 0x356: {
            Battery.voltage = this->scaleValue(this->readSignedInt16(), 0.01);
            Battery.current = this->scaleValue(this->readSignedInt16(), 0.1);
            Battery.temperature = this->scaleValue(this->readSignedInt16(), 0.1);

#ifdef PYLONTECH_DEBUG_ENABLED
            Hoymiles.getMessageOutput()->printf("[Pylontech] voltage: %f current: %f temperature: %f\n",
                Battery.voltage, Battery.current, Battery.temperature);
#endif
            break;
        }

        case 0x359: {
            uint16_t alarmBits = this->readUnsignedInt8();
            Battery.alarmOverCurrentDischarge = this->getBit(alarmBits, 7);
            Battery.alarmUnderTemperature = this->getBit(alarmBits, 4);
            Battery.alarmOverTemperature = this->getBit(alarmBits, 3);
            Battery.alarmUnderVoltage = this->getBit(alarmBits, 2);
            Battery.alarmOverVoltage= this->getBit(alarmBits, 1);

            alarmBits = this->readUnsignedInt8();
            Battery.alarmBmsInternal= this->getBit(alarmBits, 3);
            Battery.alarmOverCurrentCharge = this->getBit(alarmBits, 0);

#ifdef PYLONTECH_DEBUG_ENABLED
            Hoymiles.getMessageOutput()->printf("[Pylontech] Alarms: %d %d %d %d %d %d %d\n",
                Battery.alarmOverCurrentDischarge,
                Battery.alarmUnderTemperature,
                Battery.alarmOverTemperature,
                Battery.alarmUnderVoltage,
                Battery.alarmOverVoltage,
                Battery.alarmBmsInternal,
                Battery.alarmOverCurrentCharge);
#endif

            uint16_t warningBits = this->readUnsignedInt8();
            Battery.warningHighCurrentDischarge = this->getBit(warningBits, 7);
            Battery.warningLowTemperature = this->getBit(warningBits, 4);
            Battery.warningHighTemperature = this->getBit(warningBits, 3);
            Battery.warningLowVoltage = this->getBit(warningBits, 2);
            Battery.warningHighVoltage = this->getBit(warningBits, 1);

            warningBits = this->readUnsignedInt8();
            Battery.warningBmsInternal= this->getBit(warningBits, 3);
            Battery.warningHighCurrentCharge = this->getBit(warningBits, 0);

#ifdef PYLONTECH_DEBUG_ENABLED
            Hoymiles.getMessageOutput()->printf("[Pylontech] Warnings: %d %d %d %d %d %d %d\n",
                Battery.warningHighCurrentDischarge,
                Battery.warningLowTemperature,
                Battery.warningHighTemperature,
                Battery.warningLowVoltage,
                Battery.warningHighVoltage,
                Battery.warningBmsInternal,
                Battery.warningHighCurrentCharge);
#endif
            break;
        }

        case 0x35E: {

            String manufacturer = CAN.readString();

            if (manufacturer == "") {
                break;
            }

            strlcpy(Battery.manufacturer, manufacturer.c_str(), sizeof(Battery.manufacturer));

#ifdef PYLONTECH_DEBUG_ENABLED
            Hoymiles.getMessageOutput()->printf("[Pylontech] Manufacturer: %s\n", manufacturer.c_str());
#endif   
            break;
        }

        case 0x35C: {
            uint16_t chargeStatusBits = this->readUnsignedInt8();
            Battery.chargeEnabled = this->getBit(chargeStatusBits, 7);
            Battery.dischargeEnabled = this->getBit(chargeStatusBits, 6);
            Battery.chargeImmediately = this->getBit(chargeStatusBits, 5);

#ifdef PYLONTECH_DEBUG_ENABLED
            Hoymiles.getMessageOutput()->printf("[Pylontech] chargeStatusBits: %d %d %d\n",
                Battery.chargeEnabled,
                Battery.dischargeEnabled,
                Battery.chargeImmediately);
#endif

            this->readUnsignedInt8();
            break;
        }
    }
}

uint8_t PylontechCanReceiverClass::readUnsignedInt8()
{
    return CAN.read();
}

uint16_t PylontechCanReceiverClass::readUnsignedInt16()
{
    uint8_t bytes[2];
    bytes[0] = (uint8_t)CAN.read();
    bytes[1] = (uint8_t)CAN.read();

    return (bytes[1] << 8) + bytes[0];
}

int16_t PylontechCanReceiverClass::readSignedInt16()
{
    return this->readUnsignedInt16();
}

float PylontechCanReceiverClass::scaleValue(int16_t value, float factor)
{
    return value * factor;
}

bool PylontechCanReceiverClass::getBit(uint8_t value, uint8_t bit)
{
    return (value & (1 << bit)) >> bit;
}
