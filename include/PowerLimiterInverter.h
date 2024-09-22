// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <Hoymiles.h>
#include <optional>
#include <memory>

class PowerLimiterInverter {
public:
    static std::unique_ptr<PowerLimiterInverter> create(bool verboseLogging, PowerLimiterInverterConfig const& config);

    // send command(s) to inverter to reach desired target state (limit and
    // production). return true if an update is pending, i.e., if the target
    // state is NOT yet reached, false otherwise.
    bool update();

    // returns the timestamp of the oldest stats received for this inverter
    // *after* its last command completed. return std::nullopt if new stats
    // are pending after the last command completed.
    std::optional<uint32_t> getLatestStatsMillis() const;

    // the amount of times an update command issued to the inverter timed out
    uint8_t getUpdateTimeouts() const { return _updateTimeouts; }

    // maximum amount of AC power the inverter is able to produce
    // (not regarding the configured upper power limit)
    uint16_t getInverterMaxPowerWatts() const;

    // maximum amount of AC power the inverter is allowed to produce as per
    // upper power limit (additionally restricted by inverter's absolute max)
    uint16_t getConfiguredMaxPowerWatts() const;

    uint16_t getCurrentOutputAcWatts() const;

    // this differs from current output power if new limit was assigned
    uint16_t getExpectedOutputAcWatts() const;

    // the maximum reduction of power output the inverter
    // can achieve with or withouth going into standby.
    virtual uint16_t getMaxReductionWatts(bool allowStandby) const = 0;

    // the maximum increase of power output the inverter can achieve
    // (is expected to achieve), possibly coming out of standby.
    virtual uint16_t getMaxIncreaseWatts() const = 0;

    // change the target limit such that the requested change becomes effective
    // on the expected AC power output. returns the change in the range
    // [0..reduction] that will become effective (once update() returns false).
    virtual uint16_t applyReduction(uint16_t reduction, bool allowStandby) = 0;
    virtual uint16_t applyIncrease(uint16_t increase) = 0;

    // stop producing AC power. returns the change in power output
    // that will become effective (once update() returns false).
    virtual uint16_t standby() = 0;

    // wake the inverter from standby and set it to produce
    // as much power as permissible by its upper power limit.
    void setMaxOutput();

    void restart();

    float getDcVoltage(uint8_t input);
    bool isSendingCommandsEnabled() const { return _spInverter->getEnableCommands(); }
    bool isReachable() const { return _spInverter->isReachable(); }
    bool isProducing() const { return _spInverter->isProducing(); }

    uint64_t getSerial() const { return _config.Serial; }
    char const* getSerialStr() const { return _serialStr; }
    bool isBehindPowerMeter() const { return _config.IsBehindPowerMeter; }
    virtual bool isSolarPowered() const = 0;

    void debug() const;

protected:
    PowerLimiterInverter(bool verboseLogging, PowerLimiterInverterConfig const& config);

    // returns false if the inverter cannot participate
    // in achieving the requested change in power output
    bool isEligible() const;

    uint16_t getCurrentLimitWatts() const;

    void setTargetPowerLimitWatts(uint16_t power) { _oTargetPowerLimitWatts = power; }
    void setTargetPowerState(bool enable) { _oTargetPowerState = enable; }
    void setExpectedOutputAcWatts(uint16_t power) { _expectedOutputAcWatts = power; }

    // copied to avoid races with web UI
    PowerLimiterInverterConfig _config;

    // Hoymiles lib inverter instance
    std::shared_ptr<InverterAbstract> _spInverter = nullptr;

    bool _verboseLogging;
    char _logPrefix[32];

private:
    virtual void setAcOutput(uint16_t expectedOutputWatts) = 0;

    char _serialStr[16];

    // track (target) state
    uint8_t _updateTimeouts = 0;
    std::optional<uint32_t> _oUpdateStartMillis = std::nullopt;
    std::optional<uint16_t> _oTargetPowerLimitWatts = std::nullopt;
    std::optional<bool> _oTargetPowerState = std::nullopt;
    mutable std::optional<uint32_t> _oStatsMillis = std::nullopt;

    // the expected AC output (possibly is different from the target limit)
    uint16_t _expectedOutputAcWatts = 0;
};
