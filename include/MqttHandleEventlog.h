// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <Hoymiles.h>
#include <TaskSchedulerDeclarations.h>
#include <array>
#include <ctime>

/**
 * Publishes the per-inverter event log (alarm log) as a single retained
 * JSON array on `[base-topic][inverter-serial]/eventlog`.
 *
 * Opt-in: does nothing unless `Mqtt.EventlogEnabled` is set (default off),
 * so an unconfigured device behaves exactly like upstream OpenDTU.
 *
 * The event log itself is NOT polled here -- this class only serializes the
 * very same datastore that `WebApi_eventlog` (`/api/eventlog/status`)
 * serves, i.e. `inv->EventLog()`, which `InverterSettings` keeps up to date.
 */
class MqttHandleEventlogClass {
public:
    MqttHandleEventlogClass();
    void init(Scheduler& scheduler);

private:
    void loop();

    static String buildPayload(std::shared_ptr<InverterAbstract> inv, const time_t nowEpoch, const struct tm& nowLocal);
    static String toIso8601Utc(const time_t epoch);
    static uint32_t hashPayload(const String& payload);

    Task _loopTask;

    // Publish only on change: last payload hash per inverter POSITION (same
    // indexing scheme as MqttHandleInverterClass::_lastPublishStats).
    // `_hashValid` avoids treating a legitimate hash of 0 as "never sent".
    std::array<uint32_t, INV_MAX_COUNT> _lastPayloadHash = {};
    std::array<bool, INV_MAX_COUNT> _hashValid = {};

    // Change detection must not survive a broker session: the retained
    // eventlog lives on the broker, not in this process. After a reconnect
    // (new broker, changed base topic, or the setting being toggled -- all
    // of which end in MqttSettings.performReconnect()) the topic may hold
    // nothing at all, while the alarm log itself has not changed. Without
    // this flag the hash would still match and the topic would stay empty
    // until the next real inverter event. Cleared on the
    // disconnected->connected edge, see loop().
    bool _wasConnected = false;
};

extern MqttHandleEventlogClass MqttHandleEventlog;
