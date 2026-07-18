// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * InverterAbstract subclass for WiFi-connected Hoymiles HMS inverters.
 * Communicates via TCP/protobuf instead of RF radio.
 */
#pragma once

#include "DtuTcpClient.h"
#include <inverters/InverterAbstract.h>
#include <memory>

class HmsWifiInverter : public InverterAbstract {
public:
    explicit HmsWifiInverter(const uint64_t serial, const char* dtuIp);

    // --- InverterAbstract interface ---
    String typeName() const override;
    const byteAssign_t* getByteAssignment() const override;
    uint8_t getByteAssignmentSize() const override;

    bool isWifiInverter() const override { return true; }
    void tick() override;

    // Polling / command stubs — WiFi inverter drives its own TCP cycle
    bool sendStatsRequest() override               { return true; }
    bool sendAlarmLogRequest(bool /*force*/) override { return true; }
    bool sendDevInfoRequest() override             { return true; }
    bool sendSystemConfigParaRequest() override    { return true; }
    bool sendGridOnProFileParaRequest() override   { return true; }
    bool supportsPowerDistributionLogic() override { return false; }

    bool sendActivePowerControlRequest(float limit, PowerLimitControlType type) override;
    bool resendActivePowerControlRequest() override;
    bool sendPowerControlRequest(bool turnOn) override;
    bool resendPowerControlRequest() override;
    bool sendRestartControlRequest() override;

    // Model name reported by the DTU (from AppInfo response)
    String wifiModelName() const { return _wifiModelName; }

private:
    void _onData(const DtuData_t& data);
    void _onConnect(bool connected);
    void _populateDevInfo(const DtuData_t& d);
    void _populateAlarms(const DtuData_t& d);

    static uint8_t _detectPvCount(uint64_t serial);

    std::unique_ptr<DtuTcpClient> _tcpClient;

    // Number of DC strings (1, 2, or 4) — detected from serial prefix
    uint8_t _pvCount;

    // Pointers to the selected byteAssignment variant
    const byteAssign_t* _byteAssignment;
    uint8_t             _byteAssignmentSize;

    // Last power-limit/state for resend
    float _lastPowerLimit   = -1.0f;
    bool  _lastPowerState   = true;
    bool  _hasPendingPower  = false;
    bool  _hasPendingLimit  = false;

    String _wifiModelName;
};
