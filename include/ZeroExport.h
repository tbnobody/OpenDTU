// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "ZeroExportGridSource.h"
#include <TaskSchedulerDeclarations.h>
#include <espMqttClient.h>
#include <atomic>
#include <memory>
#include <optional>
#include <vector>

class InverterAbstract;

class ZeroExportClass {
public:
    ZeroExportClass();
    // Register the scheduler task, network-event handler, and MQTT commands.
    void init(Scheduler& scheduler);

    // Request applyConfig() from another task context (for example the Web
    // API): the request is recorded and executed by the Zero-Export loop,
    // which runs in the main-loop task.
    void requestApplyConfig() { _applyConfigPending.store(true); }

    // Current grid power from the active source, or empty if unavailable.
    std::optional<float> getGridPower() const { return activeSource().getPower(); }
    // Sum of the absolute limits most recently requested by this controller.
    float getProductionLimit() const { return _totalLimit; }
    bool isSourceRunning() const { return activeSource().isRunning(); }
    uint32_t getGridPowerAgeMs() const { return activeSource().getPowerAgeMs(); }
    // Freshness timeout currently applied to the active source's samples, in
    // milliseconds. Exposed for diagnostics: it mirrors the configured
    // FailsafeTimeout once applyConfig() has (re)started the source.
    uint32_t getGridPowerFailsafeMs() const { return activeSource().getPowerTimeoutMs(); }
    // EWMA of the interval between the active source's accepted samples, in
    // milliseconds. Returns 0 until two samples have been received.
    uint32_t getSourceUpdatePeriodMs() const { return activeSource().getUpdatePeriodMs(); }
    bool isGridFailSafeActive() const { return _gridFailSafeActive; }
    bool isEnabled() const { return _enabled; }
    int16_t getSetPoint() const { return _setPoint; }

    // Minimal per-inverter cache: everything else (rated power, reachability,
    // command enablement, reported limit) is read directly from the inverter
    // objects, which OpenDTU's poll loop keeps up to date. Public for the
    // diagnostics consumers (status API); only ever mutated by the
    // Zero-Export loop in the main-loop task.
    struct InverterState {
        uint64_t Serial = 0; // Identity at this position; detects reordered or changed inverters.
        float Limit = 0; // Last absolute limit requested by Zero-Export, in watts.
        // A new/unknown inverter is not commanded until its currently active
        // limit is known through the regular SystemConfigPara poll: the first
        // Zero-Export command then adopts that limit instead of guessing.
        // This is only used at DTU startup, after a release, or when an
        // inverter is newly seen/replaced. A temporarily unreachable inverter
        // keeps its last commanded limit instead of being reset when it
        // returns.
        bool StartupPending = true;
        // A release command sent 100% immediately before re-enabling
        // Zero-Export means startup must resume from nominal power instead of
        // sending the minimum-production initialization command again.
        bool StartupAtFullPower = false;
        // A release to 100% still needs to be sent to this inverter. This
        // remains pending while the inverter is temporarily unreachable.
        bool ReleasePending = false;
        // Tracks that a 100% release has been sent (and acknowledged by the
        // inverter becoming reachable) during the current Zero-Export disabled
        // session. It is cleared on every enable so the next disable releases
        // again, and it is reset when an inverter is replaced. Without this
        // flag, a reboot while Zero-Export is disabled would never mark the
        // inverters for release, leaving them stuck at a stale non-persistent
        // limit.
        bool Released = false;
        // Set when a grid-source outage starts and cleared only after the
        // minimum-production fail-safe command is sent to this inverter. This
        // is per inverter because reachability can change independently.
        bool FailSafePending = false;
    };

    // Per-inverter states, indexed by Hoymiles position. Read-only view for
    // diagnostics; the vector is only resized by the Zero-Export loop.
    const std::vector<InverterState>& getInverterStates() const { return _invStates; }

private:
    // Estimated delay between sending a limit command and seeing its effect
    // in the next grid measurement. Used only for source-adaptive ramping.
    static constexpr uint32_t InverterResponseMs = 2 * 1000UL;
    // Constrain a setpoint to the range accepted by the web UI and the HA
    // number entity (±10000 W), regardless of the channel it came from.
    // Pure function without instance state.
    static constexpr int16_t clampSetPoint(int16_t setPoint) { return setPoint > 10000 ? 10000 : (setPoint < -10000 ? -10000 : setPoint); }

    // Select the source matching the configuration. Falls back to the Shelly
    // LNM source if the configured source is not implemented.
    ZeroExportGridSource& activeSource() const;
    // Stop every known grid source. Used whenever the configuration or the
    // network changes so switching to another source never leaves the
    // previous one running (open socket, active subscription).
    void stopAllSources();
    // Keep the per-inverter state vector aligned with Hoymiles positions.
    size_t ensureInverterStates();

    // Apply configuration changes and network-event restarts requested by
    // other task contexts (MQTT command handlers, Web API, network events),
    // and apply them to the in-RAM configuration. Runs in the main-loop task:
    // this is the only place applyConfig() is called from
    void processPendingConfigChanges();
    // Restart the active grid source and reset cached regulation state after
    // a config change. Main-loop task only.
    void applyConfig();

    // Run the fast task, poll the grid source, and perform time-gated regulation.
    void loop();
    // Record a network (WiFi) state change observed in the WiFi event task.
    void onNetworkEvent();
    // Reconcile the cached limits with the real inverter state on every call
    // (before the grid-value and interval gates: the reconciliation itself
    // is free of RF traffic) and perform the time-gated regulation.
    void regulate();
    // Mark every inverter for the minimum-production fail-safe when the grid
    // source had a value and it expired. regulate() sends and retries the
    // commands from the inverter state machine; the low limit is deliberately
    // not kept alive: the regulation loop re-applies limits as soon as fresh
    // grid values are available again.
    void enterFailSafe(size_t numInverters);
    // Send/retry the fail-safe's per-inverter commands: the configured home
    // minimal consumption (a total production target) is distributed
    // proportionally across the inverters pending the command, falling back
    // to the per-inverter floor when it is 0. Called by regulate() after
    // enterFailSafe(), once the active grid source's value has expired.
    void applyFailSafeLimits(size_t numInverters);
    // Mark every command-enabled inverter for a relative 100% release.
    // Called once by ZeroExport.cpp's loop() on the enabled -> disabled
    // transition.
    void releaseLimits(size_t numInverters);
    // Send/retry the pending 100% release commands from the inverter state
    // machine. regulate() calls this every tick while Zero-Export is disabled.
    void applyReleaseLimits(size_t numInverters);
    // Publish HA state topics at a low, fixed rate.
    void publishStatus();
    // Register MQTT commands used by HA.
    void subscribeTopics();
    void onMqttMessageEnabled(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len);
    void onMqttMessageSetPoint(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len);

    Task _loopTask; // 500 ms task used to poll the grid source and gate regulation.

    // Indexed by Hoymiles position, but each entry also stores Serial so a
    // changed inverter order cannot reuse another inverter's cached limit.
    std::vector<InverterState> _invStates;
    float _totalLimit = 0; // Sum of the limits currently tracked by Zero-Export.
    int16_t _setPoint = 0;
    bool _enabled = false;
    bool _wasEnabled = false; // True while regulation is active; detects disable to release limits.
    uint32_t _lastRegulation = 0; // millis() timestamp of the last control calculation.
    bool _gridFailSafeActive = false; // Source value expired; limits were reduced to the minimum.
    uint32_t _lastStatusPublish = 0; // MQTT status publication throttle.
    bool _mqttWasConnected = false; // Detect reconnects and MQTT configuration changes.

    // Pending requests set from other task contexts (MQTT client task,
    // async_tcp task, WiFi event task) and consumed by loop() (main-loop
    // task). The handlers only parse, validate and log the received value:
    // taking the configuration write guard from the MQTT task would block
    // it until the main loop grants it, deadlocking both when the main loop
    // in turn waits for the MQTT client.
    std::atomic<bool> _applyConfigPending { false }; // applyConfig() requested by the Web API.
    std::atomic<bool> _networkRestartPending { false }; // Network event observed; restart the grid sources.
};

extern ZeroExportClass ZeroExport;
