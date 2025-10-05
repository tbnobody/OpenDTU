// SPDX-License-Identifier: GPL-2.0-or-later

#include <Arduino.h>
#include <Configuration.h>
#include <gridcharger/huawei/HardwareInterface.h>
#include <LogHelper.h>

#undef TAG
static const char* TAG = "gridCharger";
static const char* SUBTAG = "HwIfc";

namespace GridChargers::Huawei {

void HardwareInterface::staticLoopHelper(void* context)
{
    auto pInstance = static_cast<HardwareInterface*>(context);
    static auto constexpr resetNotificationValue = pdTRUE;
    static auto constexpr notificationTimeout = pdMS_TO_TICKS(100);

    while (true) {
        ulTaskNotifyTake(resetNotificationValue, notificationTimeout);
        {
            std::unique_lock<std::mutex> lock(pInstance->_mutex);
            if (pInstance->_stopLoop) { break; }
            pInstance->loop();
        }
    }

    pInstance->_taskDone = true;

    vTaskDelete(nullptr);
}

bool HardwareInterface::startLoop()
{
    uint32_t constexpr stackSize = 4096;
    return pdPASS == xTaskCreate(HardwareInterface::staticLoopHelper,
            "HuaweiHwIfc", stackSize, this, 16/*prio*/, &_taskHandle);
}

void HardwareInterface::stopLoop()
{
    if (_taskHandle == nullptr) { return; }

    _taskDone = false;

    {
        std::unique_lock<std::mutex> lock(_mutex);
        _stopLoop = true;
    }

    xTaskNotifyGive(_taskHandle);

    while (!_taskDone) { delay(10); }
    _taskHandle = nullptr;
}

void HardwareInterface::enqueueReceivedMessage(can_message_t const& msg)
{
    {
        std::lock_guard<std::mutex> lock(_receiveQueueMutex);
        _receiveQueue.push(msg);
    }

    xTaskNotifyGive(_taskHandle);
}

bool HardwareInterface::getMessage(HardwareInterface::can_message_t& msg)
{
    std::lock_guard<std::mutex> lock(_receiveQueueMutex);

    if (_receiveQueue.empty()) { return false; }

    msg = _receiveQueue.front();
    _receiveQueue.pop();

    return true;
}

bool HardwareInterface::readBoardProperties(can_message_t const& msg)
{
    // we process multiple messages with ID 0x1081D27F and a
    // single one with ID 0x181D27E, which concludes the answer.
    uint32_t constexpr lastId = 0x1081D27E;

    if ((msg.canId | 0x1) != (lastId | 0x1)) { return false; }

    uint16_t counter = static_cast<uint16_t>(msg.valueId >> 16);

    if (msg.canId == (lastId | 0x1) && counter == 1) {
        _boardProperties = "";
        _boardPropertiesCounter = 0;
        _boardPropertiesState = StringState::Reading;
    }

    if (StringState::Reading != _boardPropertiesState) { return true; }

    if (++_boardPropertiesCounter != counter) {
        DTU_LOGW("missed message %d while reading board properties", _boardPropertiesCounter);
        _boardPropertiesState = StringState::MissedMessage;
        return true;
    }

    auto appendAscii = [this](uint32_t val) -> void {
        val &= 0xFF;
        if ((val < 0x20 || val > 0x7E) && val != 0x0A) { return; }
        if (_boardProperties.size() > 1024) {
            DTU_LOGE("board properties growing large, resetting to protect against DoS");
            _boardProperties = "";
            return;
        }
        _boardProperties.push_back(static_cast<char>(val));
    };

    appendAscii(msg.valueId >>  8);
    appendAscii(msg.valueId >>  0);
    appendAscii(msg.value   >> 24);
    appendAscii(msg.value   >> 16);
    appendAscii(msg.value   >>  8);
    appendAscii(msg.value   >>  0);

    if (msg.canId != lastId) { return true; }

    _boardPropertiesState = StringState::Complete;

    if (DTU_LOG_IS_VERBOSE) {
        ESP_LOGV(TAG, "board properties:");
        size_t pos = 0;
        size_t next;
        while ((next = _boardProperties.find('\n', pos)) != std::string::npos) {
            if (next > pos) {
                ESP_LOGV(TAG, "%.*s", static_cast<int>(next - pos), _boardProperties.c_str() + pos);
            }
            pos = next + 1;
        }
        if (pos < _boardProperties.length()) {
            ESP_LOGV(TAG, "%.*s", static_cast<int>(_boardProperties.length() - pos), _boardProperties.c_str() + pos);
        }
    }

    auto getProperty = [this](std::string key) -> std::string {
        key += '=';
        auto start = _boardProperties.find(key);
        if (std::string::npos == start) { return "unknown"; }
        start += key.length();
        auto end = _boardProperties.find('\n', start);
        return _boardProperties.substr(start, end - start);
    };

    auto getDescription = [this,&getProperty](size_t idx) -> std::string {
        auto description = getProperty("Description");
        size_t start = 0;
        for (size_t i = 0; i < idx; ++i) {
            start = description.find(',', start);
            if (std::string::npos == start) { return ""; }
            ++start;
        }
        auto end = description.find(',', start);
        return description.substr(start, end - start);
    };

    _upData->add<DataPointLabel::BoardType>(getProperty("BoardType"));
    _upData->add<DataPointLabel::Serial>(getProperty("BarCode"));
    _upData->add<DataPointLabel::Manufactured>(getProperty("Manufactured"));
    _upData->add<DataPointLabel::VendorName>(getProperty("VendorName"));
    _upData->add<DataPointLabel::ProductName>(getDescription(1));

    std::string descr;
    size_t i = 2;
    do {
        descr = getDescription(i++);
    } while (!descr.empty() && descr.find("ectifier") == std::string::npos);
    _upData->add<DataPointLabel::ProductDescription>(descr);

    return true;
}

bool HardwareInterface::readDeviceConfig(can_message_t const& msg)
{
    // we will receive five messages with CAN ID 0x1081507F,
    // and one (the last one) with ID 0x1081507E.
    if ((msg.canId | 0x1) != 0x1081507F) { return false; }

    uint16_t counter = static_cast<uint16_t>(msg.valueId >> 16);

    if (counter == 1) {
        // device-specific value containing double the maximum output current
        // (not quite for a R4830 it seems, but the derived multiplier is spot on)
        uint8_t maxAmps = (msg.value >> 16) & 0xFF;
        _maxCurrentMultiplier = 2048 / static_cast<float>(maxAmps);
        DTU_LOGD("max current multiplier is %.2f", _maxCurrentMultiplier);
    }
    else if (counter == 6) {
        _upData->add<DataPointLabel::Row>(static_cast<uint8_t>((msg.valueId >> 8) & 0xFF));
        _upData->add<DataPointLabel::Slot>(static_cast<uint8_t>(msg.valueId & 0xFF));
        _upData->add<DataPointLabel::Reachable>(true, true/*ignore age*/);

        _lastDeviceConfigMillis = millis();
    }

    return true;
}

void HardwareInterface::requestDeviceConfig()
{
    _sendQueue.push(command_t {
        .tries = 1,
        .deviceAddress = 1,
        .registerAddress = 0x50FE,
        .command = 0,
        .flags = 0,
        .value = 0
    });
}

bool HardwareInterface::readRectifierState(can_message_t const& msg) const
{
    // we will receive a bunch of messages with CAN ID 0x1081407F,
    // and one (the last one) with ID 0x1081407E.
    if ((msg.canId | 0x1) != 0x1081407F) { return false; }

    uint32_t valueId = msg.valueId;

    // sometimes the last bit of the value ID of a message with CAN ID
    // 0x1081407E is set. TODO(schlimmchen): why?
    if (msg.canId == 0x1081407E && (valueId & 0x01) > 0) {
        DTU_LOGI("last bit in value ID %08x is set, resetting", valueId);
        valueId &= ~(0x01);
    }

    // for unknown reasons, the input voltage value ID has the last two bits
    // set on a R4830G1. this unit supports DC input as well, but these bits
    // do not change when powered the unit using DC. TODO(schlimmchen): why?
    if (msg.canId == 0x1081407F && (valueId & 0x03) > 0) {
        DTU_LOGI("last two bits in value ID %08x are set, resetting", valueId);
        valueId &= ~(0x03);
    }

    // during start-up and when shortening or opening the slot detect pins,
    // the value ID starts with 0x31 rather than 0x01. TODO(schlimmchen): why?
    if ((valueId >> 24) == 0x31) {
        DTU_LOGI("processing value for value ID starting with 0x31");
        valueId &= 0x0FFFFFFF;
    }

    if ((valueId & 0xFF00FFFF) != 0x01000000) { return false; }

    auto label = static_cast<DataPointLabel>((valueId & 0x00FF0000) >> 16);

    float divisor = 1024;

    if (label == DataPointLabel::OutputCurrentMax) {
        if (_maxCurrentMultiplier == 0) {
            DTU_LOGW("cannot process output current max value while respective multiplier unknown");
            return false;
        }
        divisor = _maxCurrentMultiplier;
    }

    float value = static_cast<float>(msg.value)/divisor;
    switch (label) {
        case DataPointLabel::InputPower:
            _upData->add<DataPointLabel::InputPower>(value);
            break;
        case DataPointLabel::InputFrequency:
            _upData->add<DataPointLabel::InputFrequency>(value);
            break;
        case DataPointLabel::InputCurrent:
            _upData->add<DataPointLabel::InputCurrent>(value);
            break;
        case DataPointLabel::OutputPower:
            _upData->add<DataPointLabel::OutputPower>(value);
            break;
        case DataPointLabel::Efficiency:
            value *= 100;
            _upData->add<DataPointLabel::Efficiency>(value);
            break;
        case DataPointLabel::OutputVoltage:
            _upData->add<DataPointLabel::OutputVoltage>(value);
            break;
        case DataPointLabel::OutputCurrentMax:
            _upData->add<DataPointLabel::OutputCurrentMax>(value);
            break;
        case DataPointLabel::InputVoltage:
            _upData->add<DataPointLabel::InputVoltage>(value);
            break;
        case DataPointLabel::OutputTemperature:
            _upData->add<DataPointLabel::OutputTemperature>(value);
            break;
        case DataPointLabel::InputTemperature:
            _upData->add<DataPointLabel::InputTemperature>(value);
            break;
        case DataPointLabel::OutputCurrent:
            _upData->add<DataPointLabel::OutputCurrent>(value);
            break;
        default:
            uint8_t rawLabel = static_cast<uint8_t>(label);
            // 0x0E/0x0A seems to be a static label/value pair, so we don't print it
            if (rawLabel != 0x0E || msg.value != 0x0A) {
                DTU_LOGV("raw value for 0x%02x is 0x%08x (%d), scaled by 1024: %.2f, scaled by %.2f: %.2f",
                        rawLabel, msg.value, msg.value,
                        static_cast<float>(msg.value)/1024,
                        _maxCurrentMultiplier,
                        static_cast<float>(msg.value)/std::max(_maxCurrentMultiplier, 1.0f));
            }
            break;
    }

    return true;
}

bool HardwareInterface::readAcks(can_message_t const& msg) const
{
    if (msg.canId != 0x1081807e) { return false; }

    uint32_t valueId = msg.valueId;

    auto setting = static_cast<Setting>(valueId >> 16);
    auto flags = valueId & 0x0000FFFF;

    float divisor = 1024;

    if (setting == Setting::OnlineCurrent || setting == Setting::OfflineCurrent) {
        if (_maxCurrentMultiplier == 0) {
            DTU_LOGW("cannot process %s current setting while respective multiplier unknown",
                    setting == Setting::OnlineCurrent ? "online" : "offline");
            return false;
        }
        divisor = _maxCurrentMultiplier;
    }

    float value = static_cast<float>(msg.value)/divisor;

    switch (setting) {
        case Setting::OnlineVoltage:
            _upData->add<DataPointLabel::OnlineVoltage>(value);
            break;
        case Setting::OfflineVoltage:
            _upData->add<DataPointLabel::OfflineVoltage>(value);
            break;
        case Setting::OnlineCurrent:
            _upData->add<DataPointLabel::OnlineCurrent>(value);
            break;
        case Setting::OfflineCurrent:
            _upData->add<DataPointLabel::OfflineCurrent>(value);
            break;
        case Setting::InputCurrentLimit:
            _upData->add<DataPointLabel::InputCurrentLimit>(value);
            break;
        case Setting::ProductionDisable:
            _upData->add<DataPointLabel::ProductionEnabled>((flags & 0x0001) == 0);
            break;
        case Setting::FanOnlineFullSpeed:
            _upData->add<DataPointLabel::FanOnlineFullSpeed>((flags & 0x0001) > 0);
            break;
        case Setting::FanOfflineFullSpeed:
            _upData->add<DataPointLabel::FanOfflineFullSpeed>((flags & 0x0001) > 0);
            break;
    }

    return true;
}

void HardwareInterface::sendSettings()
{
    auto const& config = Configuration.get().GridCharger;

    using Setting = HardwareInterface::Setting;
    enqueueParameter(Setting::OfflineVoltage, config.Huawei.OfflineVoltage);
    enqueueParameter(Setting::OfflineCurrent, config.Huawei.OfflineCurrent);
    enqueueParameter(Setting::InputCurrentLimit, config.Huawei.InputCurrentLimit);
    enqueueParameter(Setting::FanOnlineFullSpeed, config.Huawei.FanOnlineFullSpeed ? 1 : 0);
    enqueueParameter(Setting::FanOfflineFullSpeed, config.Huawei.FanOfflineFullSpeed ? 1 : 0);

    _lastSettingsUpdateMillis = millis();
}

void HardwareInterface::logMessage(char const* msg, uint32_t canId, uint32_t valueId, uint32_t value)
{
    if (!DTU_LOG_IS_VERBOSE) { return; }

    char buffer[70] = {0};
    int offset = 0;
    auto save_snprintf = [&offset, &buffer](auto&&... args) -> bool {
        int written = snprintf(buffer + offset, sizeof(buffer) - offset, std::forward<decltype(args)>(args)...);
        if (written < 0 || written >= static_cast<int>(sizeof(buffer)) - offset) {
            DTU_LOGE("snprintf issue: wrote %d bytes, offset is %d, buffer size is %d, buffered '%s'",
                    written, offset, sizeof(buffer), buffer);
            return false;
        }
        offset += written;
        return true;
    };
    if (!save_snprintf("%9s: address %08x ID %08x value %08x | ",
            msg, canId, valueId, value)) { return; }

    auto printAscii = [&save_snprintf](uint32_t value) -> bool{
        for (int i = 24; i >= 0; i -= 8) {
            uint8_t byte = (value >> i) & 0xFF;
            if (byte >= 0x20 && byte <= 0x7E) {
                if (!save_snprintf("%c", byte)) { return false; }
            } else {
                if (!save_snprintf(".")) { return false; }
            }
        }
        return true;
    };

    if (!printAscii(valueId)) { return; }
    if (!save_snprintf(" ")) { return; }
    if (!printAscii(value)) { return; }

    DTU_LOGV("%s", buffer);
}

void HardwareInterface::loop()
{
    can_message_t msg;

    if (!_upData) { _upData = std::make_unique<DataPointContainer>(); }

    while (getMessage(msg)) {
        if (readBoardProperties(msg) ||
                readDeviceConfig(msg) ||
                readRectifierState(msg) ||
                readAcks(msg)) {
            logMessage("processed", msg.canId, msg.valueId, msg.value);
            continue;
        }

        // examples for codes not handled are:
        //     0x1001117E (Whr meter),
        //     0x100011FE (unclear), 0x108111FE (output enabled),
        //     0x108081FE (unclear).
        // https://github.com/craigpeacock/Huawei_R4850G2_CAN/blob/main/r4850.c
        // https://www.beyondlogic.org/review-huawei-r4850g2-power-supply-53-5vdc-3kw/
        logMessage("ignored", msg.canId, msg.valueId, msg.value);
    }

    // the first thing we need to do is to request the device config so we know
    // the max current multiplier. that is required to process the ACK for
    // the offline current setting, or even send that setting in particular.
    if (!_lastDeviceConfigMillis) {
        // stand by while processing the device config request and not timed out
        if ((millis() - _lastRequestMillis) < 5000) { return; }

        requestDeviceConfig();

        _lastRequestMillis = millis();

        return processQueue();
    }

    if ((millis() - *_lastDeviceConfigMillis) > DeviceConfigTimeoutMillis) {
        DTU_LOGW("PSU is unreachable (no CAN communication)");
        _maxCurrentMultiplier = 0;
        _lastDeviceConfigMillis = std::nullopt;
        _lastSettingsUpdateMillis = std::nullopt;
        _boardPropertiesState = StringState::Unknown;
        _upData->add<DataPointLabel::Reachable>(false, true/*ignore age*/);
        return; // restart by re-requesting device config in next iteration
    }

    if (!_lastSettingsUpdateMillis) {
        sendSettings();
        return processQueue();
    }

    if (StringState::Complete != _boardPropertiesState) {
        // stand by while processing the board properties replies and not timed out
        if (StringState::Reading == _boardPropertiesState &&
                (millis() - _lastRequestMillis) < 5000) { return; }

        _lastRequestMillis = millis();

        _sendQueue.push(command_t {
            .tries = 1,
            .deviceAddress = 1,
            .registerAddress = 0xD2FE,
            .command = 0,
            .flags = 0,
            .value = 0
        });

        return processQueue(); // not sending timed requests until we know the board properties
    }

    if ((millis() - _lastRequestMillis) >= DataRequestIntervalMillis) {
        // we request the device config regularly as the row and index (slot detection)
        // might change by use of the "power" pin, and we use the device config to
        // determine whether the PSU is reachable.
        requestDeviceConfig();

        // request rectifier state
        _sendQueue.push(command_t {
            .tries = 1,
            .deviceAddress = 1,
            .registerAddress = 0x40FE,
            .command = 0,
            .flags = 0,
            .value = 0
        });

        _lastRequestMillis = millis();

        return processQueue();
    }
}

void HardwareInterface::processQueue()
{
    size_t queueSize = _sendQueue.size();
    for (size_t i = 0; i < queueSize; ++i) {
        auto& cmd = _sendQueue.front();

        std::array<uint8_t, 8> data = {
            static_cast<uint8_t>((cmd.command >> 8) & 0xFF),
            static_cast<uint8_t>((cmd.command >> 0) & 0xFF),
            static_cast<uint8_t>((cmd.flags >>  8) & 0xFF),
            static_cast<uint8_t>((cmd.flags >>  0) & 0xFF),
            static_cast<uint8_t>((cmd.value >> 24) & 0xFF),
            static_cast<uint8_t>((cmd.value >> 16) & 0xFF),
            static_cast<uint8_t>((cmd.value >>  8) & 0xFF),
            static_cast<uint8_t>((cmd.value >>  0) & 0xFF)
        };

        uint32_t addr = 0x10800000 | (cmd.deviceAddress << 16) | cmd.registerAddress;

        uint32_t valueId = (static_cast<uint32_t>(cmd.command) << 16) | cmd.flags;
        logMessage("sending", addr, valueId, cmd.value);

        if (sendMessage(addr, data)) {
            _sendQueue.pop();
            continue;
        }

        if (cmd.tries > 0) { --cmd.tries; }

        DTU_LOGE("Sending to 0x%08x failed (no CAN ACK), command 0x%04x, "
                "flags 0x%04x, value 0x%08x, %d tries remaining",
                addr, cmd.command, cmd.flags, cmd.value, cmd.tries);

        if (cmd.tries == 0) { _sendQueue.pop(); }
    }
}

void HardwareInterface::enqueueParameter(HardwareInterface::Setting setting, float val)
{
    uint16_t flags = 0;

    switch (setting) {
        case Setting::OfflineVoltage:
        case Setting::OnlineVoltage:
            val *= 1024;
            break;
        case Setting::OfflineCurrent:
        case Setting::OnlineCurrent:
            if (_maxCurrentMultiplier == 0) {
                DTU_LOGW("max current multiplier unknown, cannot send current setting");
                return;
            }
            val *= _maxCurrentMultiplier;
            break;
        case Setting::InputCurrentLimit:
            val *= 1024;
            if (val > 0) {
                flags = 0x0001;
            }
            break;
        case Setting::FanOnlineFullSpeed:
        case Setting::FanOfflineFullSpeed:
        case Setting::ProductionDisable:
            if (val > 0) {
                flags = 0x0001;
            }
            val = 0;
            break;
    }

    _sendQueue.push(command_t {
        .tries = 3,
        .deviceAddress = 1,
        .registerAddress = 0x80FE,
        .command = static_cast<uint16_t>(setting),
        .flags = flags,
        .value = static_cast<uint32_t>(val)
    });
}

void HardwareInterface::setParameter(HardwareInterface::Setting setting, float val, bool pollFeedback)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_taskHandle == nullptr) { return; }

    enqueueParameter(setting, val);

    if (pollFeedback) { // request early param feedback
        _lastRequestMillis = millis() - DataRequestIntervalMillis;
    }

    xTaskNotifyGive(_taskHandle);
}

std::unique_ptr<DataPointContainer> HardwareInterface::getCurrentData()
{
    std::unique_ptr<DataPointContainer> upData = nullptr;

    {
        std::lock_guard<std::mutex> lock(_mutex);
        upData = std::move(_upData);
    }

    if (upData && DTU_LOG_IS_DEBUG) {
        auto iter = upData->cbegin();
        while (iter != upData->cend()) {
            DTU_LOGD("[%.3f] %s: %s%s",
                static_cast<float>(iter->second.getTimestamp())/1000,
                iter->second.getLabelText().c_str(),
                iter->second.getValueText().c_str(),
                iter->second.getUnitText().c_str());
            ++iter;
        }
    }

    return std::move(upData);
}

} // namespace GridChargers::Huawei
