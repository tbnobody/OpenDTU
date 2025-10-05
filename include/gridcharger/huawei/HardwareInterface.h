// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <atomic>
#include <array>
#include <mutex>
#include <memory>
#include <queue>
#include <string>
#include <tuple>
#include <cstdint>
#include <gridcharger/huawei/DataPoints.h>

namespace GridChargers::Huawei {

class HardwareInterface {
public:
    HardwareInterface() = default;

    virtual ~HardwareInterface() = default;

    virtual bool init() = 0;

    enum class Setting : uint16_t {
        OnlineVoltage = 0x0100,
        OfflineVoltage = 0x0101,
        OnlineCurrent = 0x0103,
        OfflineCurrent = 0x0104,
        InputCurrentLimit = 0x0109,
        ProductionDisable = 0x0132,
        FanOnlineFullSpeed = 0x0134,
        FanOfflineFullSpeed = 0x0135
    };
    void setParameter(Setting setting, float val, bool pollFeedback = false);

    std::unique_ptr<DataPointContainer> getCurrentData();

    static uint32_t constexpr DataRequestIntervalMillis = 2500;

protected:
    struct CAN_MESSAGE_T {
        uint32_t canId;
        uint32_t valueId;
        int32_t value;
    };
    using can_message_t = struct CAN_MESSAGE_T;

    bool startLoop();
    void stopLoop();

    void enqueueReceivedMessage(can_message_t const& msg);

private:
    static void staticLoopHelper(void* context);
    static void logMessage(char const* msg, uint32_t canId, uint32_t valueId, uint32_t value);
    void loop();
    void processQueue();

    bool getMessage(can_message_t& msg);

    std::mutex _receiveQueueMutex;
    std::queue<can_message_t> _receiveQueue;

    virtual bool sendMessage(uint32_t canId, std::array<uint8_t, 8> const& data) = 0;

    mutable std::mutex _mutex;

    TaskHandle_t _taskHandle = nullptr;
    std::atomic<bool> _taskDone = false;
    bool _stopLoop = false;

    std::unique_ptr<DataPointContainer> _upData = nullptr;

    struct COMMAND {
        uint8_t tries;
        uint8_t deviceAddress;
        uint16_t registerAddress;
        uint16_t command;
        uint16_t flags;
        uint32_t value;
    };
    using command_t = struct COMMAND;
    std::queue<command_t> _sendQueue;

    float _maxCurrentMultiplier = 0; // device-specific, must be fetched first

    uint32_t _lastRequestMillis = 0;

    bool readBoardProperties(can_message_t const& msg);

    std::string _boardProperties = "";
    uint16_t _boardPropertiesCounter = 0;
    enum class StringState : uint8_t {
        Unknown,
        RequestFailed,
        Reading,
        MissedMessage,
        Complete
    };
    StringState _boardPropertiesState = StringState::Unknown;
    uint32_t _boardPropertiesRequestMillis = 0;

    bool readDeviceConfig(can_message_t const& msg);
    void requestDeviceConfig();
    std::optional<uint32_t> _lastDeviceConfigMillis = std::nullopt;
    static constexpr uint32_t DeviceConfigTimeoutMillis = DataRequestIntervalMillis * 4;

    bool readRectifierState(can_message_t const& msg) const;

    bool readAcks(can_message_t const& msg) const;

    std::optional<uint32_t> _lastSettingsUpdateMillis = std::nullopt;
    void sendSettings();

    void enqueueParameter(Setting setting, float val);
};

} // namespace GridChargers::Huawei
