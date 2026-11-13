// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Zero-Export controller.
 *
 * Reads the grid power reported by a configured grid source (see
 * ZeroExportGridSource) and regulates the inverter power limits so that
 * the grid power stays around a configurable setpoint (default: 0 W,
 * i.e. no export to / no import from the grid).
 *
 * The regulation only depends on the incoming grid value, the setpoint and
 * the limits currently applied to the inverters. It does NOT take the
 * inverters' production values into account, because these can be outdated.
 */
#include "Configuration.h"
#include "Datastore.h"
#include "MqttHandleHass.h"
#include "MqttSettings.h"
#include "ZeroExport.h"
#include "ZeroExport_MQTT.h"
#include "ZeroExport_ShellyLNM.h"
#include "defaults.h"
#include <Hoymiles.h>
#include <functional>

#undef TAG
static const char* TAG = "zeroexport";

#define ZEROEXPORT_PRODUCTION_MARGIN_MIN 300.0f // W, minimum headroom above measured production
#define ZEROEXPORT_PRODUCTION_MARGIN_PERCENT 0.30f // 30%, additional headroom above measured production

// Share of a total power budget proportional to an inverter's rated power
// among a group whose combined rated power is nominalTotal, clamped to the
// inverter's own capacity and never below floor. Used both by the regular
// regulation and by the fail-safe distribution of the configured home
// minimal consumption. nominalTotal must be > 0.
static float distributeProportional(uint16_t maxPower, uint16_t nominalTotal, float target, uint16_t floor)
{
    const float weight = static_cast<float>(maxPower) / static_cast<float>(nominalTotal);
    const float share = std::ceil(target * weight);
    return std::min<float>(maxPower, std::max<float>(static_cast<float>(floor), share));
}

size_t ZeroExportClass::ensureInverterStates()
{
    const size_t numInverters = Hoymiles.getNumInverters();
    if (_invStates.size() != numInverters) {
        _invStates.resize(numInverters);
    }
    return numInverters;
}

void ZeroExportClass::releaseLimits(size_t numInverters)
{
    ESP_LOGI(TAG, "Releasing inverter limits (Zero-Export disabled)");
    for (size_t i = 0; i < numInverters; i++) {
        auto inv = Hoymiles.getInverterByPos(i);
        if (inv == nullptr) {
            continue;
        }

        auto& state = _invStates[i];
        state.Serial = inv->serial();
        state.Limit = 0;
        state.StartupPending = true;
        state.StartupAtFullPower = false;
        state.Released = false;
        state.ReleasePending = inv->getEnableCommands();
    }

    // All tracked limits were cleared by the release: the published total
    // must not keep showing the last regulated value while disabled.
    _totalLimit = 0;
}

void ZeroExportClass::applyReleaseLimits(size_t numInverters)
{
    // Release is part of the per-inverter state machine. A pending
    // inverter is retried on each regulation tick until it is reachable
    // and command-enabled; completed releases are never resent.
    for (size_t i = 0; i < numInverters; i++) {
        auto inv = Hoymiles.getInverterByPos(i);
        auto& state = _invStates[i];
        if (inv == nullptr || !state.ReleasePending) {
            continue;
        }
        if (state.Serial != inv->serial()) {
            state.Serial = inv->serial();
            state.Limit = 0;
            state.StartupPending = true;
            state.StartupAtFullPower = false;
            state.Released = false;
            state.ReleasePending = inv->getEnableCommands();
        }
        if (!inv->getEnableCommands()) {
            ESP_LOGD(TAG, "Release skipped for %s: commands disabled", inv->name());
            state.ReleasePending = false;
            continue;
        }
        if (!inv->isReachable()) {
            ESP_LOGD(TAG, "Release pending for %s: not reachable", inv->name());
            continue;
        }

        ESP_LOGI(TAG, "Release limit for %s: 100%%", inv->name());
        inv->sendActivePowerControlRequest(100, PowerLimitControlType::RelativNonPersistent);
        state.StartupAtFullPower = true;
        state.ReleasePending = false;
        state.Released = true;
    }
}

void ZeroExportClass::enterFailSafe(size_t numInverters)
{
    // Mark every inverter for the fail-safe. The actual commands are sent
    // and retried by regulate(), which owns the inverter state machine and
    // covers inverters that are temporarily unreachable when they return.
    for (auto& state : _invStates) {
        state.FailSafePending = true;
    }
    _gridFailSafeActive = true;
}

void ZeroExportClass::applyFailSafeLimits(size_t numInverters)
{
    const CONFIG_T& config = Configuration.get();

    // Fail-safe is part of the per-inverter state machine. A pending
    // inverter is retried on each regulation tick until it is reachable
    // and command-enabled; completed fail-safes are never resent on a
    // timer: if the inverter's own non-persistent-limit timeout reverts
    // it to full power, the regulation loop readjusts as soon as fresh
    // grid values are available again.
    //
    // A newly seen or replaced inverter (for example one added while the
    // outage was already active, after enterFailSafe() marked the other
    // states) must also receive the fail-safe command. This pass also
    // sums the rated power of the inverters pending the command, so the
    // configured home minimal consumption (a total production target)
    // can be distributed proportionally across them instead of dropping
    // every inverter straight to its bare floor. A HomeMinimalConsumption
    // of 0 makes the target 0 for every inverter, which collapses to the
    // per-inverter floor exactly like the previous flat-floor behavior.
    uint16_t pendingNominal = 0;
    for (size_t i = 0; i < numInverters; i++) {
        auto inv = Hoymiles.getInverterByPos(i);
        if (inv == nullptr) {
            continue;
        }
        auto& st = _invStates[i];
        if (st.Serial != inv->serial()) {
            st.Serial = inv->serial();
            st.Limit = 0;
            st.StartupPending = true;
            st.FailSafePending = true;
        }
        if (!st.FailSafePending || !inv->getEnableCommands() || !inv->isReachable()) {
            continue;
        }
        const uint16_t maxPower = inv->DevInfo()->getMaxPower();
        if (maxPower == 0) {
            continue;
        }
        pendingNominal += maxPower;
    }

    bool commandSent = false;
    for (size_t i = 0; i < numInverters; i++) {
        auto inv = Hoymiles.getInverterByPos(i);
        if (inv == nullptr) {
            continue;
        }
        auto& st = _invStates[i];
        if (!st.FailSafePending) {
            continue;
        }
        if (!inv->getEnableCommands() || !inv->isReachable()) {
            // Do not reset the cached limit for an inverter that cannot
            // receive the fail-safe command: its last commanded limit
            // remains the source of truth until it becomes reachable.
            continue;
        }
        const uint16_t maxPower = inv->DevInfo()->getMaxPower();
        if (maxPower == 0) {
            continue;
        }

        const float target = std::min<float>(pendingNominal, config.ZeroExport.HomeMinimalConsumption);
        const uint16_t safeLimit = static_cast<uint16_t>(distributeProportional(maxPower, pendingNominal, target, config.ZeroExport.InverterMinimalPower));
        ESP_LOGW(TAG, "Grid source unavailable: limiting %s to %" PRIu16 " W", inv->name(), safeLimit);
        inv->sendActivePowerControlRequest(safeLimit, PowerLimitControlType::AbsolutNonPersistent);
        st.Limit = safeLimit;
        st.StartupPending = false;
        st.StartupAtFullPower = false;
        st.FailSafePending = false;
        commandSent = true;
    }

    if (commandSent) {
        _totalLimit = 0;
        for (const auto& st : _invStates) {
            _totalLimit += st.Limit;
        }
    }
}

void ZeroExportClass::regulate()
{
    const CONFIG_T& config = Configuration.get();
    const size_t numInverters = ensureInverterStates();

    if (!_enabled) {
        // While Zero-Export is disabled the inverters must be kept at 100%.
        // Mark every command-enabled inverter that has not been released in
        // this disabled session as pending. This covers the disable
        // transition, a reboot while disabled, and inverters that were
        // unreachable at the moment Zero-Export was turned off.
        for (size_t i = 0; i < numInverters; i++) {
            auto inv = Hoymiles.getInverterByPos(i);
            auto& state = _invStates[i];
            if (inv == nullptr) {
                continue;
            }
            if (state.Serial != inv->serial()) {
                state.Serial = inv->serial();
                state.Limit = 0;
                state.StartupPending = true;
                state.StartupAtFullPower = false;
                state.Released = false;
            }
            if (!state.Released && inv->getEnableCommands()) {
                state.ReleasePending = true;
            }
        }
        applyReleaseLimits(numInverters);
        return;
    }

    ZeroExportGridSource& source = activeSource();

    // No grid value means no regulation can be performed. Check this before
    // touching inverter state or sending startup limit commands: there is no
    // reason to generate RF traffic while the grid source is unavailable.
    // The next valid source value will start the commanded-limit sequence.
    // (Re-)size state tracking first, so _totalLimit is never left at a stale
    // default when no inverters are present yet.
    const std::optional<float> gridPowerValue = source.getPower();

    if (!gridPowerValue.has_value()) {
        // A source that has not delivered a single value since it was started
        // (module just enabled, source just switched, network just
        // reconnected, or the source cannot run at all) is a startup
        // condition, not a measurement outage: no regulation and no
        // fail-safe, the inverters' current limits are left untouched so
        // enabling Zero-Export never yanks producing inverters down to the
        // minimum. The fail-safe below only covers a measurement that existed
        // and expired.
        if (!source.hasReceivedValue()) {
            return;
        }

        // The source had a value and it expired: enter the fail-safe, which
        // sets the inverters to a low limit once but does not bother keeping
        // it alive. The regulation loop takes over again as soon as fresh
        // grid values arrive.
        if (!_gridFailSafeActive) {
            ESP_LOGW(TAG, "No fresh grid value: applying minimum production fail-safe");
            enterFailSafe(numInverters);
        }

        applyFailSafeLimits(numInverters);
        return;
    }

    const float gridPower = gridPowerValue.value();

    // Grid power is available, so the fail-safe state can be cleared.
    _gridFailSafeActive = false;
    for (auto& state : _invStates) {
        state.FailSafePending = false;
    }

    if (numInverters == 0) {
        _invStates.clear();
        _totalLimit = 0;
        return;
    }

    // Derive the regulation aggregates from the last limits requested by this
    // controller. OpenDTU's reported limits are deliberately not used here:
    // the grid error is the feedback signal. (The one-time startup adoption
    // below is the only exception.)
    //
    // Startup/recovery rules:
    // - A DTU reboot with Zero-Export active adopts the limit currently
    //   active in each inverter once it is known through the regular
    //   SystemConfigPara poll. No Zero-Export command is sent while it is
    //   still unknown: the startup is retried later.
    // - Re-enabling after releaseLimits() starts at nominal power because the
    //   last command sent was already a 100% release.
    // - A newly seen/replaced inverter adopts its current limit as well.
    // - A temporarily unreachable inverter keeps its last commanded limit and
    //   is included as fixed capacity when the other inverters regulate.
    // The startup command is sent only after the grid source has a value, so
    // an unavailable source cannot cause unnecessary inverter commands.
    uint16_t nominalTotal = 0;
    float fixedLimit = 0; // limits belonging to currently unreachable inverters
    uint16_t controllableNominal = 0;
    size_t controllableCount = 0;
    float totalLimit = 0;
    uint16_t floorSum = 0; // Sum of the per-inverter floors the distribution below enforces.
    bool startupCommandSent = false;

    for (uint8_t i = 0; i < numInverters; i++) {
        auto inv = Hoymiles.getInverterByPos(i);
        auto& st = _invStates[i];

        const uint16_t maxPower = (inv == nullptr) ? 0 : inv->DevInfo()->getMaxPower();
        if (maxPower == 0) {
            st.Serial = 0;
            st.Limit = 0;
            st.StartupPending = true;
            continue;
        }

        if (st.Serial != inv->serial()) {
            st.Serial = inv->serial();
            st.Limit = 0;
            st.StartupPending = true;
        }

        const bool controllable = inv->isReachable() && inv->getEnableCommands();
        if (st.StartupPending) {
            if (!controllable) {
                continue; // Initialize when the inverter becomes controllable.
            }

            // Determine the starting limit. When the limit currently active
            // in the inverter is known (through the regular SystemConfigPara
            // poll or an acknowledged limit command), adopt it: regulation
            // then starts from reality instead of guessing, which could
            // dump a producing inverter's output or leave it unrestrained.
            // Until it is known, retry on the next run. OpenDTU's Hoymiles
            // poll loop requests SystemConfigPara automatically, so no
            // Zero-Export command is sent before the limit is known.
            float startLimit = 0;
            const char* startReason = "";

            if (st.StartupAtFullPower) {
                // The last Zero-Export command released the inverter to
                // 100%: nominal power is the correct starting point.
                startLimit = maxPower;
                startReason = " (after release)";
            } else if (inv->SystemConfigPara()->getLastUpdate() > 0) {
                const float reportedLimit = inv->SystemConfigPara()->getLimitPercent() * maxPower / 100.0f;
                if (reportedLimit > 0) {
                    startLimit = std::min<float>(maxPower, reportedLimit);
                    startReason = " (adopting current limit)";
                }
            }

            if (startLimit <= 0) {
                // The inverter's active limit is not known yet. Do not guess
                // a limit or send a Zero-Export command: OpenDTU's regular
                // Hoymiles poll loop will request SystemConfigPara and this
                // startup path retries after its response arrives.
                continue;
            }

            st.Limit = startLimit;
            st.StartupPending = false;
            st.StartupAtFullPower = false;
            startupCommandSent = true;
            ESP_LOGI(TAG, "Startup limit for %s: %.0f W%s", inv->name(), st.Limit, startReason);
            inv->sendActivePowerControlRequest(st.Limit, PowerLimitControlType::AbsolutNonPersistent);
        }

        nominalTotal += maxPower;
        totalLimit += st.Limit;
        if (controllable) {
            controllableNominal += maxPower;
            controllableCount++;
            floorSum += std::min<uint16_t>(config.ZeroExport.InverterMinimalPower, maxPower);
        } else {
            // Keep a known command limit fixed while an inverter is offline or
            // commands are disabled (for example during the night).
            fixedLimit += st.Limit;
        }
    }

    _totalLimit = totalLimit;

    // Let the startup commands take effect before using their new limits for
    // the first regulation calculation: the inverter needs a moment to apply
    // the limit and the grid source to measure the result. The first
    // regulation run therefore happens after the configured interval, like
    // every other one.
    if (startupCommandSent) {
        _lastRegulation = millis();
        return;
    }

    if (millis() - _lastRegulation < config.ZeroExport.UpdateInterval * 1000UL) {
        return; // Respect the configured regulation interval
    }
    _lastRegulation = millis();

    if (controllableCount == 0 || nominalTotal <= 0) {
        ESP_LOGD(TAG, "No controllable inverters available");
        return;
    }

    // Regulation (grid power sign convention: positive = power drawn from
    // the grid, negative = feed-in, like Shelly's act_power):
    //   delta = (grid - setpoint)   ; grid > setpoint -> importing -> increase limit
    //                                     ; grid < setpoint -> exporting -> decrease limit
    float delta = gridPower - _setPoint;

    // Source-adaptive correction: with a slow source (for example an MQTT
    // meter publishing every 10-30 seconds), the previous correction may not
    // have reached the grid measurement yet. Applying the full error again
    // would multiply the effective loop gain and can cause overshoot. Use the
    // measured source period to spread upward corrections over the source's
    // response time. Downward corrections stay immediate to reduce export.
    const uint32_t sourcePeriodMs = source.getUpdatePeriodMs();
    if (delta > 0 && sourcePeriodMs > 0) {
        const uint32_t loopLatencyMs = sourcePeriodMs + InverterResponseMs;
        const uint32_t regulationIntervalMs = config.ZeroExport.UpdateInterval * 1000UL;
        const float gain = std::min<float>(1.0f,
            static_cast<float>(regulationIntervalMs) / static_cast<float>(loopLatencyMs));
        delta = delta * gain;
        if (gain < 1.0f) {
            ESP_LOGW(TAG, "Slow grid source (period %" PRIu32 " ms): correction factor: %.0f%%",
                static_cast<uint32_t>(sourcePeriodMs), gain * 100.0f);
        }
    }

    // Do not let the requested limit drift far above the production currently
    // reported by the inverters. Keep at least 200 W of headroom so small
    // production values do not make the cap unnecessarily restrictive.
    // The cap's base is never below the enforced per-inverter floor sum: the
    // inverters cannot produce more than their currently applied limit, so a
    // morning start (measured production below the floor sum, e.g. 8 x 40 W)
    // with a cap of "production + margin" would sit below the floor sum and
    // the limit could never ramp up. Basing the cap on the floor sum instead
    // guarantees the loop can always step out of a low-light start: floor
    // 320 W -> cap 520 W, the production follows the new limit, and each
    // cycle lifts the cap again. This is not a configured "total minimum":
    // it is exactly the sum of per-inverter floors the distribution below
    // enforces anyway (over controllable inverters only).
    const float currentProduction = std::max<float>(0, Datastore.getTotalAcPowerEnabled());
    const float productionMargin = std::max<float>(
        ZEROEXPORT_PRODUCTION_MARGIN_MIN,
        currentProduction * ZEROEXPORT_PRODUCTION_MARGIN_PERCENT);
    const float productionBase = std::max<float>(currentProduction, floorSum);
    const float productionCap = std::min<float>(
        nominalTotal,
        productionBase + productionMargin);

    // Compute the new total limit by applying the delta to the current total limit.
    float newTotalLimit = _totalLimit + delta;
    newTotalLimit = std::min<float>(nominalTotal, newTotalLimit);
    newTotalLimit = std::min<float>(newTotalLimit, productionCap);
    newTotalLimit = std::ceil(newTotalLimit);

    // If the adjustment is smaller than 1 W, consider it negligible and skip
    // the update; the next regulation run re-evaluates anyway.
    if (std::fabs(newTotalLimit - _totalLimit) < 1.0f) {
        ESP_LOGD(TAG, "Grid %.1f W, setpoint %" PRId16 " W -> no adjustment needed (total limit %.0f W)",
            gridPower, _setPoint, _totalLimit);
        return;
    }

    ESP_LOGI(TAG, "Grid %.1f W, setpoint %" PRId16 " W -> adjusting total limit from %.0f to %.0f W",
        gridPower, _setPoint, _totalLimit, newTotalLimit);

    // Keep unreachable inverters fixed and distribute only the adjustable
    // portion proportionally to reachable inverters. This handles both a
    // temporary communication outage and an inverter that remains offline.
    const float adjustableNominal = std::max<float>(0, nominalTotal - fixedLimit);
    const float adjustableTarget = std::min<float>(adjustableNominal,
        std::max<float>(0, newTotalLimit - fixedLimit));

    float distributed = 0;
    for (size_t i = 0; i < numInverters; i++) {
        auto inv = Hoymiles.getInverterByPos(i);
        auto& st = _invStates[i];

        const uint16_t maxPower = (inv == nullptr) ? 0 : inv->DevInfo()->getMaxPower();
        if (maxPower == 0 || st.StartupPending || !inv->isReachable() || !inv->getEnableCommands()) {
            if (maxPower > 0 && !st.StartupPending) {
                // Known commanded limit, but currently not controllable: keep
                // it fixed in the total while other inverters compensate.
                distributed += st.Limit;
            }
            continue;
        }

        const float target = distributeProportional(maxPower, controllableNominal, adjustableTarget, config.ZeroExport.InverterMinimalPower);
        // Never command an inverter below a hard floor. The configured total
        // minimum can be split into very small per-inverter shares; a too-low
        // absolute limit can make an inverter stop producing entirely.
        // (Enforced by distributeProportional() above.)

        // Correct rounding drift on the last relevant inverter is not
        // necessary: the error is at most a few watts and gets fixed by the
        // next regulation cycle.

        if (std::fabs(target - st.Limit) >= 1.0f) {
            // Never enqueue a power control command for an inverter that is
            // not reachable: the radio layer retries unacknowledged commands
            // for a long time, which blocks the command queue and delays
            // commands for all other inverters.
            if (!inv->isReachable()) {
                ESP_LOGW(TAG, "%s: limit update skipped, inverter not reachable", inv->name());
                distributed += st.Limit;
                continue;
            }
            ESP_LOGI(TAG, "%s: %.0f -> %.0f W (max %" PRIu16 " W)",
                inv->name(), st.Limit, target, maxPower);
            inv->sendActivePowerControlRequest(target, PowerLimitControlType::AbsolutNonPersistent);
        }
        st.Limit = target;
        distributed += target;
    }

    _totalLimit = distributed;
}
