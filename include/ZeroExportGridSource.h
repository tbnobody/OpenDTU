// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>
#include <cmath>
#include <cstdint>
#include <optional>

// Abstract grid power source used by the Zero-Export controller.
//
// A source receives the current grid power from some device (power meter,
// energy sensor, ...) and exposes the most recent value to the controller.
// Implementations own their transport (UDP socket, HTTP client, ...) and
// are expected to update their value asynchronously; the controller only
// polls them and never blocks.
class ZeroExportGridSource {
public:
    ZeroExportGridSource() = default;
    virtual ~ZeroExportGridSource() = default;

    void setPowerTimeoutMs(uint32_t powerTimeoutMs) { _powerTimeoutMs = powerTimeoutMs; }

    // Prepare the source (open sockets, join groups, ...). Called whenever
    // the configuration changes or the network comes up. Safe to call when
    // already started.
    virtual void start() = 0;
    // Release the transport resources. Safe to call when already stopped.
    virtual void stop() = 0;
    // Whether the transport is currently up (socket joined / client
    // connected). Used by the controller to self-heal a failed start.
    virtual bool isRunning() const = 0;
    // Called on every controller tick. The implementation may drain received
    // packets, fetch data, or perform any other non-blocking maintenance.
    virtual void loop() = 0;

    // Current grid power in watts, or empty when no value is available or the
    // implementation-specific freshness timeout has expired.
    std::optional<float> getPower() const
    {
        if (std::isnan(_power) || _lastUpdate == 0 || millis() - _lastUpdate > _powerTimeoutMs) {
            return std::nullopt;
        }
        return _power;
    }

    // Whether at least one value has been accepted since the last stop().
    // Lets the controller distinguish a source that is still waiting for
    // its first value (just started or reconnected) from one whose value
    // went stale after having been valid (measurement outage).
    bool hasReceivedValue() const { return _lastUpdate != 0; }

    uint32_t getPowerTimeoutMs() const { return _powerTimeoutMs; }
    uint32_t getPowerAgeMs() const { return _lastUpdate == 0 ? 0 : millis() - _lastUpdate; }

    // Estimated source update period in milliseconds, calculated as an EWMA
    // of intervals between accepted samples. Returns 0 until two samples are
    // available. The controller uses it to adapt correction size: a source
    // publishing every second can be corrected faster than one publishing
    // every 30 seconds.
    uint32_t getUpdatePeriodMs() const { return _updatePeriodMs; }

protected:
    // Store an accepted measurement and update its timestamp/period estimate.
    void setPower(float power)
    {
        const uint32_t nowMs = millis();
        _power = power;
        if (_lastSampleMs == 0) {
            _lastSampleMs = nowMs;
        } else {
            const uint32_t interval = nowMs - _lastSampleMs;
            if (interval == 0 || interval > PERIOD_OUTLIER_MS) {
                // Zero interval (burst) or gap larger than the freshness
                // timeout: not representative of the source cadence.
                _lastSampleMs = nowMs;
            } else {
                if (_updatePeriodMs == 0) {
                    _updatePeriodMs = interval;
                } else {
                    // EWMA: 87.5% previous estimate, 12.5% new interval.
                    _updatePeriodMs = (_updatePeriodMs * 7UL + interval) / 8UL;
                }
                _lastSampleMs = nowMs;
            }
        }
        _lastUpdate = nowMs;
    }

    // Clear the measurement and reset its timestamp/period estimate.
    void clearPower()
    {
        _power = NAN;
        _lastUpdate = 0;
        _lastSampleMs = 0;
        _updatePeriodMs = 0;
    }

private:
    static constexpr uint32_t PERIOD_OUTLIER_MS = 10 * 60 * 1000UL; // 10 min: ignore burst/gap intervals

    uint32_t _lastSampleMs = 0; // millis() of the previous accepted sample.
    uint32_t _updatePeriodMs = 0; // EWMA of the sample interval, in ms.
    uint32_t _powerTimeoutMs = 5 * 1000UL; // Freshness timeout supplied by the source during start().
    uint32_t _lastUpdate = 0; // millis() timestamp of _power.
    float _power = NAN; // Most recent accepted power measurement.
};
