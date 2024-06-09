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

    _upSdmSerial = std::make_unique<SoftwareSerial>();
    _upSdm = std::make_unique<SDM>(*_upSdmSerial, 9600, pin.powermeter_dere,
            SWSERIAL_8N1, pin.powermeter_rx, pin.powermeter_tx);
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
    mqttPublish("voltage1", _phase1Voltage);
    mqttPublish("import", _energyImport);
    mqttPublish("export", _energyExport);

    if (_phases == Phases::Three) {
        mqttPublish("power2", _phase2Power);
        mqttPublish("power3", _phase3Power);
        mqttPublish("voltage2", _phase2Voltage);
        mqttPublish("voltage3", _phase3Voltage);
    }
}

void PowerMeterSerialSdm::pollingLoopHelper(void* context)
{
    auto pInstance = static_cast<PowerMeterSerialSdm*>(context);
    pInstance->pollingLoop();
    pInstance->_taskDone = true;
    vTaskDelete(nullptr);
}

bool PowerMeterSerialSdm::readValue(std::unique_lock<std::mutex>& lock, uint16_t reg, float& targetVar)
{
    lock.unlock(); // reading values takes too long to keep holding the lock
    float val = _upSdm->readVal(reg, _cfg.Address);
    lock.lock();

    // we additionally check in between each transaction whether or not we are
    // actually asked to stop polling altogether. otherwise, the destructor of
    // this instance might need to wait for a whole while until the task ends.
    if (_stopPolling) { return false; }

    auto err = _upSdm->getErrCode(true/*clear error code*/);

    switch (err) {
        case SDM_ERR_NO_ERROR:
            if (_verboseLogging) {
                MessageOutput.printf("[PowerMeterSerialSdm]: read register %d "
                        "(0x%04x) successfully\r\n", reg, reg);
            }

            targetVar = val;
            return true;
            break;
        case SDM_ERR_CRC_ERROR:
            MessageOutput.printf("[PowerMeterSerialSdm]: CRC error "
                    "while reading register %d (0x%04x)\r\n", reg, reg);
            break;
        case SDM_ERR_WRONG_BYTES:
            MessageOutput.printf("[PowerMeterSerialSdm]: unexpected data in "
                    "message while reading register %d (0x%04x)\r\n", reg, reg);
            break;
        case SDM_ERR_NOT_ENOUGHT_BYTES:
            MessageOutput.printf("[PowerMeterSerialSdm]: unexpected end of "
                    "message while reading register %d (0x%04x)\r\n", reg, reg);
            break;
        case SDM_ERR_TIMEOUT:
            MessageOutput.printf("[PowerMeterSerialSdm]: timeout occured "
                    "while reading register %d (0x%04x)\r\n", reg, reg);
            break;
        default:
            MessageOutput.printf("[PowerMeterSerialSdm]: unknown SDM error "
                    "code after reading register %d (0x%04x)\r\n", reg, reg);
            break;
    }

    return false;
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

        // reading takes a "very long" time as each readVal() is a synchronous
        // exchange of serial messages. cache the values and write later to
        // enforce consistent values.
        float phase1Power = 0.0;
        float phase2Power = 0.0;
        float phase3Power = 0.0;
        float phase1Voltage = 0.0;
        float phase2Voltage = 0.0;
        float phase3Voltage = 0.0;
        float energyImport = 0.0;
        float energyExport = 0.0;

        bool success = readValue(lock, SDM_PHASE_1_POWER, phase1Power) &&
            readValue(lock, SDM_PHASE_1_VOLTAGE, phase1Voltage) &&
            readValue(lock, SDM_IMPORT_ACTIVE_ENERGY, energyImport) &&
            readValue(lock, SDM_EXPORT_ACTIVE_ENERGY, energyExport);

        if (success && _phases == Phases::Three) {
            success = readValue(lock, SDM_PHASE_2_POWER, phase2Power) &&
                readValue(lock, SDM_PHASE_3_POWER, phase3Power) &&
                readValue(lock, SDM_PHASE_2_VOLTAGE, phase2Voltage) &&
                readValue(lock, SDM_PHASE_3_VOLTAGE, phase3Voltage);
        }

        if (!success) { continue; }

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
