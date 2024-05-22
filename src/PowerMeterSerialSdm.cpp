// SPDX-License-Identifier: GPL-2.0-or-later
#include "PowerMeterSerialSdm.h"
#include "Configuration.h"
#include "PinMapping.h"
#include "MessageOutput.h"
#include "SerialPortManager.h"

PowerMeterSerialSdm::~PowerMeterSerialSdm()
{
    if (_upSdmSerial) {
        _upSdmSerial->end();
        _upSdmSerial = nullptr;
    }
}

bool PowerMeterSerialSdm::init()
{
    const PinMapping_t& pin = PinMapping.get();

    MessageOutput.printf("[PowerMeterSerialSdm] rx = %d, tx = %d, dere = %d\r\n",
            pin.powermeter_rx, pin.powermeter_tx, pin.powermeter_dere);

    if (pin.powermeter_rx < 0 || pin.powermeter_tx < 0) {
        MessageOutput.println("[PowerMeterSerialSdm] invalid pin config for SDM "
                "power meter (RX and TX pins must be defined)");
        return false;
    }

    auto oHwSerialPort = SerialPortManager.allocatePort(_sdmSerialPortOwner);
    if (!oHwSerialPort) { return false; }

    _upSdmSerial = std::make_unique<HardwareSerial>(*oHwSerialPort);
    _upSdmSerial->end(); // make sure the UART will be re-initialized
    _upSdm = std::make_unique<SDM>(*_upSdmSerial, 9600, pin.powermeter_dere,
            SERIAL_8N1, pin.powermeter_rx, pin.powermeter_tx);
    _upSdm->begin();

    return true;
}

float PowerMeterSerialSdm::getPowerTotal() const
{
    std::lock_guard<std::mutex> l(_mutex);
    return _phase1Power + _phase2Power + _phase3Power;
}

void PowerMeterSerialSdm::doMqttPublish() const
{
    std::lock_guard<std::mutex> l(_mutex);
    mqttPublish("power1", _phase1Power);
    mqttPublish("power2", _phase2Power);
    mqttPublish("power3", _phase3Power);
    mqttPublish("voltage1", _phase1Voltage);
    mqttPublish("voltage2", _phase2Voltage);
    mqttPublish("voltage3", _phase3Voltage);
    mqttPublish("import", _energyImport);
    mqttPublish("export", _energyExport);
}

void PowerMeterSerialSdm::loop()
{
    if (!_upSdm) { return; }

    auto const& config = Configuration.get();

    if ((millis() - _lastPoll) < (config.PowerMeter.SerialSdm.PollingInterval * 1000)) {
        return;
    }

    uint8_t addr = config.PowerMeter.SerialSdm.Address;

    // reading takes a "very long" time as each readVal() is a synchronous
    // exchange of serial messages. cache the values and write later to
    // enforce consistent values.
    float phase1Power = _upSdm->readVal(SDM_PHASE_1_POWER, addr);
    float phase2Power = 0.0;
    float phase3Power = 0.0;
    float phase1Voltage = _upSdm->readVal(SDM_PHASE_1_VOLTAGE, addr);
    float phase2Voltage = 0.0;
    float phase3Voltage = 0.0;
    float energyImport = _upSdm->readVal(SDM_IMPORT_ACTIVE_ENERGY, addr);
    float energyExport = _upSdm->readVal(SDM_EXPORT_ACTIVE_ENERGY, addr);

    if (static_cast<PowerMeterProvider::Type>(config.PowerMeter.Source) == PowerMeterProvider::Type::SDM3PH) {
        phase2Power = _upSdm->readVal(SDM_PHASE_2_POWER, addr);
        phase3Power = _upSdm->readVal(SDM_PHASE_3_POWER, addr);
        phase2Voltage = _upSdm->readVal(SDM_PHASE_2_VOLTAGE, addr);
        phase3Voltage = _upSdm->readVal(SDM_PHASE_3_VOLTAGE, addr);
    }

    {
        std::lock_guard<std::mutex> l(_mutex);
        _phase1Power = static_cast<float>(phase1Power);
        _phase2Power = static_cast<float>(phase2Power);
        _phase3Power = static_cast<float>(phase3Power);
        _phase1Voltage = static_cast<float>(phase1Voltage);
        _phase2Voltage = static_cast<float>(phase2Voltage);
        _phase3Voltage = static_cast<float>(phase3Voltage);
        _energyImport = static_cast<float>(energyImport);
        _energyExport = static_cast<float>(energyExport);
    }

    gotUpdate();

    MessageOutput.printf("[PowerMeterSerialSdm] TotalPower: %5.2f\r\n", getPowerTotal());

    _lastPoll = millis();
}
