// SPDX-License-Identifier: GPL-2.0-or-later
#include "PowerMeterSerialSdm.h"
#include "PinMapping.h"
#include "MessageOutput.h"
#include "SerialPortManager.h"

PowerMeterSerialSdm::~PowerMeterSerialSdm()
{
    _taskDone = false;

    std::unique_lock<std::mutex> lock(_pollingMutex);
    _stopPolling = true;
    lock.unlock();

    _cv.notify_all();

    if (_taskHandle != nullptr) {
        while (!_taskDone) { delay(10); }
        _taskHandle = nullptr;
    }

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

    std::unique_lock<std::mutex> lock(_pollingMutex);
    _stopPolling = false;
    lock.unlock();

    uint32_t constexpr stackSize = 3072;
    xTaskCreate(PowerMeterSerialSdm::pollingLoopHelper, "PM:SDM",
            stackSize, this, 1/*prio*/, &_taskHandle);

    return true;
}

float PowerMeterSerialSdm::getPowerTotal() const
{
    std::lock_guard<std::mutex> l(_valueMutex);
    return _phase1Power + _phase2Power + _phase3Power;
}

bool PowerMeterSerialSdm::isDataValid() const
{
    uint32_t age = millis() - getLastUpdate();
    return getLastUpdate() > 0 && (age < (3 * _cfg.PollingInterval * 1000));
}

void PowerMeterSerialSdm::doMqttPublish() const
{
    std::lock_guard<std::mutex> l(_valueMutex);
    mqttPublish("power1", _phase1Power);
    mqttPublish("power2", _phase2Power);
    mqttPublish("power3", _phase3Power);
    mqttPublish("voltage1", _phase1Voltage);
    mqttPublish("voltage2", _phase2Voltage);
    mqttPublish("voltage3", _phase3Voltage);
    mqttPublish("import", _energyImport);
    mqttPublish("export", _energyExport);
}

void PowerMeterSerialSdm::pollingLoopHelper(void* context)
{
    auto pInstance = static_cast<PowerMeterSerialSdm*>(context);
    pInstance->pollingLoop();
    pInstance->_taskDone = true;
    vTaskDelete(nullptr);
}

void PowerMeterSerialSdm::pollingLoop()
{
    std::unique_lock<std::mutex> lock(_pollingMutex);

    while (!_stopPolling) {
        auto elapsedMillis = millis() - _lastPoll;
        auto intervalMillis = _cfg.PollingInterval * 1000;
        if (_lastPoll > 0 && elapsedMillis < intervalMillis) {
            auto sleepMs = intervalMillis - elapsedMillis;
            _cv.wait_for(lock, std::chrono::milliseconds(sleepMs),
                    [this] { return _stopPolling; }); // releases the mutex
            continue;
        }

        _lastPoll = millis();

        uint8_t addr = _cfg.Address;

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

        if (_phases == Phases::Three) {
            phase2Power = _upSdm->readVal(SDM_PHASE_2_POWER, addr);
            phase3Power = _upSdm->readVal(SDM_PHASE_3_POWER, addr);
            phase2Voltage = _upSdm->readVal(SDM_PHASE_2_VOLTAGE, addr);
            phase3Voltage = _upSdm->readVal(SDM_PHASE_3_VOLTAGE, addr);
        }

        {
            std::lock_guard<std::mutex> l(_valueMutex);
            _phase1Power = static_cast<float>(phase1Power);
            _phase2Power = static_cast<float>(phase2Power);
            _phase3Power = static_cast<float>(phase3Power);
            _phase1Voltage = static_cast<float>(phase1Voltage);
            _phase2Voltage = static_cast<float>(phase2Voltage);
            _phase3Voltage = static_cast<float>(phase3Voltage);
            _energyImport = static_cast<float>(energyImport);
            _energyExport = static_cast<float>(energyExport);
        }

        MessageOutput.printf("[PowerMeterSerialSdm] TotalPower: %5.2f\r\n", getPowerTotal());

        gotUpdate();
    }
}
