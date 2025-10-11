// SPDX-License-Identifier: GPL-2.0-or-later

#include <gridcharger/trucki/Provider.h>
#include <gridcharger/trucki/DataPoints.h>
#include <battery/Controller.h>
#include <powermeter/Controller.h>
#include <PowerLimiter.h>
#include <Utils.h>
#include <WiFiUdp.h>
#include <LogHelper.h>

#undef TAG
static const char* TAG = "gridCharger";
static const char* SUBTAG = "Trucki";

static constexpr unsigned int udpPort = 4211;  // trucki T2xG port
static WiFiUDP TruckiUdp;

namespace GridChargers::Trucki {

bool Provider::init()
{
    DTU_LOGI("Initialize Trucki AC charger interface...");

    auto const& config = Configuration.get();
    auto const& ipAddress = IPAddress(config.GridCharger.Trucki.IpAddress);

    if (ipAddress.toString() == "0.0.0.0") {
        DTU_LOGE("Invalid IP address: %s", ipAddress.toString().c_str());
        return false;
    }

    if (!TruckiUdp.begin(udpPort)) {
        DTU_LOGE("Failed to initialize UDP");
        return false;
    }

    _httpRequestConfig = std::make_unique<HttpRequestConfig>();
    strlcpy(_httpRequestConfig->Url, ("http://" + ipAddress.toString() + "/jsonlive").c_str(), sizeof(_httpRequestConfig->Url));
    _httpRequestConfig->Timeout = HTTP_REQUEST_TIMEOUT_MS;
    strlcpy(_httpRequestConfig->HeaderKey, "", sizeof(_httpRequestConfig->HeaderKey));
    strlcpy(_httpRequestConfig->HeaderValue, "", sizeof(_httpRequestConfig->HeaderValue));
    strlcpy(_httpRequestConfig->Username, "admin", sizeof(_httpRequestConfig->Username)); // default username
    strlcpy(_httpRequestConfig->Password, config.GridCharger.Trucki.Password, sizeof(_httpRequestConfig->Password));

    if (strlen(_httpRequestConfig->Password) > 0) {
        _httpRequestConfig->AuthType = HttpRequestConfig::Auth::Basic;
    } else {
        _httpRequestConfig->AuthType = HttpRequestConfig::Auth::None;
    }

    DTU_LOGD("Request URL: %s", _httpRequestConfig->Url);

    _httpGetter = std::make_unique<HttpGetter>(*_httpRequestConfig);
    _httpGetter->addHeader("Accept", "*/*");

    if (!_httpGetter->init()) {
        DTU_LOGE("Initializing HTTP getter failed: %s", _httpGetter->getErrorText());
        _httpGetter = nullptr;
        return false;
    }

    return true;
}

void Provider::deinit()
{
    TruckiUdp.stop();

    _dataPollingTaskDone = false;

    std::unique_lock pollingLock(_dataPollingMutex);
    _stopPollingData = true;
    pollingLock.unlock();

    _dataPollingCv.notify_all();

    if (_dataPollingTaskHandle != nullptr) {
        while (!_dataPollingTaskDone) { delay(10); }
        _dataPollingTaskHandle = nullptr;
    }

    _httpGetter = nullptr;
    _httpRequestConfig = nullptr;
}

void Provider::loop()
{
    powerControlLoop();

    sendControlCommandRequest();
    parseControlCommandResponse();

    if (_dataPollingTaskHandle == nullptr) {
        std::unique_lock lock(_dataPollingMutex);
        _stopPollingData = false;
        lock.unlock();

        uint32_t constexpr stackSize = 6144;
        xTaskCreate(dataPollingLoopHelper, "TruckiPolling",
                stackSize, this, 1/*prio*/, &_dataPollingTaskHandle);
    }
}

void Provider::powerControlLoop()
{
    auto& config = Configuration.get();

    auto oMaxAcPower = _dataCurrent.get<DataPointLabel::MaxAcPower>();
    auto oOutputPower = _dataCurrent.get<DataPointLabel::DcPower>();

    // ***********************
    // Emergency charge
    // ***********************
    auto stats = Battery.getStats();
    if (!_batteryEmergencyCharging && config.GridCharger.EmergencyChargeEnabled && stats->getImmediateChargingRequest()) {
        if (!oMaxAcPower) {
            // TODO(andreasboehm): if this situation actually occurs, this message
            // will be printed with high frequency for a prolonged time. how can
            // we deal with that?
            DTU_LOGW("Cannot perform emergency charging with unknown PSU max ac power value");
            return;
        }

        _batteryEmergencyCharging = true;

        DTU_LOGI("Emergency Charge AC Power %.02f", *oMaxAcPower);
        setRequestedPowerAc(*oMaxAcPower);
        return;
    }

    if (_batteryEmergencyCharging && !stats->getImmediateChargingRequest()) {
        // Battery request has changed. Set current to 0, wait for PSU to respond and then clear state
        setRequestedPowerAc(0);
        if (oOutputPower && oOutputPower < 1) {
            _batteryEmergencyCharging = false;
        }
        return;
    }

    // ***********************
    // Automatic power control
    // ***********************
    if (config.GridCharger.AutoPowerEnabled) {
        // Check if we should run automatic power calculation at all.
        // We may have set a value recently and still wait for output stabilization
        if (_autoModeBlockedTillMillis > millis()) {
            return;
        }

        if (PowerLimiter.isGovernedBatteryPoweredInverterProducing()) {
            setRequestedPowerAc(0);
            _autoPowerEnabled = false;
            DTU_LOGI("Inverter is active, disable PSU");
            _autoModeBlockedTillMillis = millis() + 1000;
            return;
        }

        if (millis() - _dataCurrent.getLastUpdate() > 30 * 1000) {
            DTU_LOGW("Cannot perform auto power control when critical PSU values are outdated");
            _autoModeBlockedTillMillis = millis() + 1000;
            return;
        }

        auto oOutputVoltage = _dataCurrent.get<DataPointLabel::DcVoltage>();
        auto oBatteryVoltageLimit = _dataCurrent.get<DataPointLabel::DcVoltageSetpoint>();
        auto oMinAcPower = _dataCurrent.get<DataPointLabel::MinAcPower>();
        auto oOutputCurrent = _dataCurrent.get<DataPointLabel::DcCurrent>();

        if (!oOutputPower || !oOutputVoltage || !oBatteryVoltageLimit || !oMinAcPower || !oMaxAcPower || !oOutputCurrent) {
            DTU_LOGW("Cannot perform auto power control while critical PSU values are still unknown");
            _autoModeBlockedTillMillis = millis() + 1000;
            return;
        }

        // Re-enable automatic power control if the output voltage has dropped below threshold
        if (oOutputVoltage && *oOutputVoltage < oBatteryVoltageLimit) {
            _autoPowerEnabled = true;
        }

        // We have received a new PowerMeter value. Also we're _autoPowerEnabled
        // So we're good to calculate a new limit
        if (PowerMeter.getLastUpdate() > _lastPowerMeterUpdateReceivedMillis && _autoPowerEnabled) {
            _lastPowerMeterUpdateReceivedMillis = PowerMeter.getLastUpdate();

            float powerTotal = round(PowerMeter.getPowerTotal());

            // Calculate new power limit
            float newPowerLimit = -1 * powerTotal;

            // Powerlimit is the current output power + permissable Grid consumption
            newPowerLimit += *oOutputPower + config.GridCharger.AutoPowerTargetPowerConsumption;

            DTU_LOGV("powerTotal: %.0f, outputPower: %.01f, newPowerLimit: %.0f", powerTotal, *oOutputPower, newPowerLimit);

            // Check whether the battery SoC limit setting is enabled
            if (config.Battery.Enabled && config.GridCharger.AutoPowerBatterySoCLimitsEnabled) {
                uint8_t _batterySoC = Battery.getStats()->getSoC();
                // Sets power limit to 0 if the BMS reported SoC reaches or exceeds the user configured value
                if (_batterySoC >= config.GridCharger.AutoPowerStopBatterySoCThreshold) {
                    newPowerLimit = 0;
                    DTU_LOGV("Current battery SoC %i reached stop threshold %i, set newPowerLimit to %f",
                            _batterySoC, config.GridCharger.AutoPowerStopBatterySoCThreshold, newPowerLimit);
                }
            }

            if (newPowerLimit >= *oMinAcPower) {
                // Limit power to maximum
                if (newPowerLimit > *oMaxAcPower) {
                    newPowerLimit = *oMaxAcPower;
                }

                auto efficiency = _dataCurrent.get<DataPointLabel::Efficiency>().value_or(90) / 100.0f;
                efficiency = efficiency > 0.5f ? efficiency : 0.9f;

                // Calculate output current
                float calculatedCurrent = efficiency * (newPowerLimit / *oOutputVoltage);

                // Limit output current to value requested by BMS
                float permissibleCurrent = stats->getChargeCurrentLimit() - (stats->getChargeCurrent() - *oOutputCurrent); // BMS current limit - current from other sources, e.g. Victron MPPT charger
                float outputCurrent = std::min(calculatedCurrent, permissibleCurrent);
                outputCurrent = outputCurrent > 0 ? outputCurrent : 0;

                // calculate new power limit based on output current
                newPowerLimit = (outputCurrent * *oOutputVoltage) / efficiency;

                _autoPowerEnabled = true;
                setRequestedPowerAc(newPowerLimit);

                // Don't run auto mode some time to allow for output stabilization after issuing a new value
                _autoModeBlockedTillMillis = millis() + 4 * DATA_POLLING_INTERVAL_MS;
            } else {
                // requested PL is below minium. Set power to 0
                _autoPowerEnabled = false;
                setRequestedPowerAc(0);
            }
        }
    }
}

void Provider::setRequestedPowerAc(float power)
{
    _requestedPowerAc = power;
}

void Provider::sendControlCommandRequest()
{
    auto& config = Configuration.get();

    if (!config.GridCharger.AutoPowerEnabled && !config.GridCharger.EmergencyChargeEnabled) {
        return;
    }

    if (millis() - _lastControlCommandRequestMillis < CONTROL_COMMAND_INTERVAL_MS) { return; }

    DTU_LOGV("Setting charging power to %.02fW AC", _requestedPowerAc);

    uint16_t acPowerSetpoint = _requestedPowerAc * 10; // ac power in W*10

    TruckiUdp.beginPacket(config.GridCharger.Trucki.IpAddress, udpPort);
    TruckiUdp.print(String(acPowerSetpoint));
    TruckiUdp.endPacket();

    _lastControlCommandRequestMillis = millis();
}

void Provider::parseControlCommandResponse()
{
    int packetSize = TruckiUdp.parsePacket();
    if (0 == packetSize) { return; }

    std::vector<char> buffer(packetSize + 1, '\0');
    int readBytes = TruckiUdp.read(buffer.data(), packetSize);
    if (readBytes <= 0) { return; }
    buffer[readBytes] = '\0'; // ensure null-terminated string

    DTU_LOGD("received %d bytes - %s", packetSize, buffer.data());

    // Parse packet format: "current_power;max_power;battery_state", e.g. "1000;1000;1"
    // - first number is the current AC power in W*10
    // - second number is the max AC power in W*10
    // - third number is the battery state and optional, not available in older versions
    float acPowerCurrent;
    float acPowerMax;
    int batteryState = -1;
    int parsedFields = sscanf(buffer.data(), "%f;%f;%d", &acPowerCurrent, &acPowerMax, &batteryState);
    if (parsedFields >= 2) {
        acPowerCurrent /= 10.0f; // Convert from W*10 to W
        acPowerMax /= 10.0f; // Convert from W*10 to W
    } else {
        DTU_LOGW("Invalid packet format: %s", buffer.data());
        return;
    }

    DTU_LOGV("acPowerCurrent: %f, acPowerMax: %f, batteryState: %d", acPowerCurrent, acPowerMax, batteryState);

    // Update data points
    {
        auto scopedLock = _dataCurrent.lock();
        _dataCurrent.add<DataPointLabel::AcPower>(acPowerCurrent);
        _dataCurrent.add<DataPointLabel::MaxAcPower>(acPowerMax);

        // only use batteryState when it could be parsed from the packet
        if (parsedFields == 3) {
            switch (batteryState) {
                case 5:
                    _dataCurrent.add<DataPointLabel::BatteryGridState>(std::string("trucki.VGRID_LOW"));
                    break;

                case 4:
                    _dataCurrent.add<DataPointLabel::BatteryGridState>(std::string("trucki.VGRID_LOW_DELAYED"));
                    break;

                case 3:
                    _dataCurrent.add<DataPointLabel::BatteryGridState>(std::string("trucki.VBAT_FULL_DELAYED"));
                    break;

                case 2:
                    _dataCurrent.add<DataPointLabel::BatteryGridState>(std::string("trucki.VBAT_FULL"));
                    break;

                case 1:
                    _dataCurrent.add<DataPointLabel::BatteryGridState>(std::string("trucki.VBAT_NORMAL"));
                    break;

                case 0:
                    _dataCurrent.add<DataPointLabel::BatteryGridState>(std::string("trucki.VBAT_LOW"));
                    break;

                default:
                    _dataCurrent.add<DataPointLabel::BatteryGridState>(std::string("trucki.UNKNOWN"));
                    break;
            }
        }
    }

    _stats->updateFrom(_dataCurrent);
}

void Provider::dataPollingLoopHelper(void* context)
{
    auto pInstance = static_cast<Provider*>(context);
    pInstance->dataPollingLoop();
    pInstance->_dataPollingTaskDone = true;
    vTaskDelete(nullptr);
}

void Provider::dataPollingLoop()
{
    std::unique_lock lock(_dataPollingMutex);

    while (!_stopPollingData) {
        auto elapsedMillis = millis() - _lastDataPoll;

        if (_lastDataPoll > 0 && elapsedMillis < DATA_POLLING_INTERVAL_MS) {
            auto sleepMs = DATA_POLLING_INTERVAL_MS - elapsedMillis;
            _dataPollingCv.wait_for(lock, std::chrono::milliseconds(sleepMs),
                    [this] { return _stopPollingData; }); // releases the mutex
            continue;
        }

        _lastDataPoll = millis();

        lock.unlock(); // polling can take quite some time
        pollData();
        lock.lock();
    }
}

void Provider::pollData()
{
    if (!_httpGetter) {
        DTU_LOGE("HTTP getter not initialized");
        return;
    }

    auto result = _httpGetter->performGetRequest();

    if (!result) {
        DTU_LOGE("Failed to get data from Trucki: %s", _httpGetter->getErrorText());
        return;
    }

    auto pStream = result.getStream();
    if (!pStream) {
        DTU_LOGE("Programmer error: HTTP request yields no stream");
        return;
    }

    JsonDocument data;
    const DeserializationError error = deserializeJson(data, *pStream);
    if (error) {
        DTU_LOGE("Unable to parse server response as JSON: %s", error.c_str());
        return;
    }

    {
        auto scopedLock = _dataCurrent.lock();

        addStringToDataPoints<DataPointLabel::ZEPC>(data, "ZEPCPOWER");
        addStringToDataPoints<DataPointLabel::State>(data, "MWPCSTATE");
        addFloatToDataPoints<DataPointLabel::Temperature>(data, "TEMP");
        addFloatToDataPoints<DataPointLabel::Efficiency>(data, "MWEFFICIENCY");
        addFloatToDataPoints<DataPointLabel::DayEnergy>(data, "DAYENERGY");
        addFloatToDataPoints<DataPointLabel::TotalEnergy>(data, "TOTALENERGY");

        addFloatToDataPoints<DataPointLabel::AcVoltage>(data, "VGRID");
        addFloatToDataPoints<DataPointLabel::AcPowerSetpoint>(data, "SETACPOWER");
        addFloatToDataPoints<DataPointLabel::AcPower>(data, "MQTT_ACDISPLAY_VALUE");

        addFloatToDataPoints<DataPointLabel::DcVoltage>(data, "VBAT");
        addFloatToDataPoints<DataPointLabel::DcVoltageSetpoint>(data, "VOUTSET");
        addFloatToDataPoints<DataPointLabel::DcPower>(data, "DCPOWER");
        addFloatToDataPoints<DataPointLabel::DcCurrent>(data, "IOUT");

        addFloatToDataPoints<DataPointLabel::DcVoltageOffline>(data, "VOUTOFFLINE");
        addFloatToDataPoints<DataPointLabel::DcCurrentOffline>(data, "IOUTOFFLINE");
        addFloatToDataPoints<DataPointLabel::MinAcPower>(data, "MQTT_MINPOWER_VALUE");
        addFloatToDataPoints<DataPointLabel::MaxAcPower>(data, "POWERLIMIT");
    }

    _stats->updateFrom(_dataCurrent);
}
} // namespace GridChargers::Trucki
