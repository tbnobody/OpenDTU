// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2026 Thomas Basler and others
 * Copyright (C) 2026 lokalKraft (fork addition: eventlog -> MQTT)
 */
#include "MqttHandleEventlog.h"
#include "MqttSettings.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <algorithm>
#include <ctime>

#undef TAG
static const char* TAG = "mqtt";

// An event that (after anchoring, see toEpoch()) lies slightly in the future
// is still treated as "today": the inverter clock and the DTU NTP clock are
// not perfectly in sync, and a few seconds of skew must not push a fresh
// event a whole day into the past.
#define EVENTLOG_FUTURE_TOLERANCE_SEC 300

#define SECONDS_PER_DAY (24 * 60 * 60)

MqttHandleEventlogClass MqttHandleEventlog;

MqttHandleEventlogClass::MqttHandleEventlogClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&MqttHandleEventlogClass::loop, this))
{
}

void MqttHandleEventlogClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);
    _loopTask.enable();
}

void MqttHandleEventlogClass::loop()
{
    const CONFIG_T& config = Configuration.get();

    // Update interval from config (same pattern as MqttHandleInverterTotal)
    _loopTask.setInterval(config.Mqtt.PublishInterval * TASK_SECOND);

    if (!config.Mqtt.EventlogEnabled) {
        return;
    }

    const bool connected = MqttSettings.getConnected();
    if (connected && !_wasConnected) {
        // Fresh broker session: forget what we believe the broker retains.
        // The next iteration republishes every inverter's current event log,
        // which is what makes a broker change / base topic change / toggling
        // the setting actually show up on the new topic.
        _hashValid.fill(false);
    }
    _wasConnected = connected;

    if (!connected || !Hoymiles.isAllRadioIdle()) {
        _loopTask.forceNextIteration();
        return;
    }

    // Real timestamps require a synchronized clock. The raw event log only
    // carries a time OF DAY (see toEpoch()), so without NTP there is no
    // honest way to build an absolute timestamp -- skip instead of
    // publishing a wrong one.
    struct tm nowLocal;
    if (!getLocalTime(&nowLocal, 5)) {
        ESP_LOGD(TAG, "Eventlog publish skipped: no NTP time yet");
        return;
    }
    const time_t nowEpoch = std::time(nullptr);

    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);
        if (inv == nullptr) {
            continue;
        }

        const String payload = buildPayload(inv, nowEpoch, nowLocal);
        const uint32_t hash = hashPayload(payload);

        // Publish only on change -- the event log is a retained topic, a
        // consumer gets the current state on connect anyway; republishing an
        // unchanged array every PublishInterval would be pure broker noise.
        if (_hashValid[i] && _lastPayloadHash[i] == hash) {
            continue;
        }
        _lastPayloadHash[i] = hash;
        _hashValid[i] = true;

        // Forced retained (NOT Configuration.Mqtt.Retain): the event log is
        // state, not a sample. A consumer connecting later must still see
        // the currently active faults.
        MqttSettings.publishGeneric(MqttSettings.getPrefix() + inv->serialString() + "/eventlog", payload, true, 0);

        yield();
    }
}

/**
 * Anchors a time OF DAY from the alarm log on a real date.
 *
 * `AlarmLogEntry_t::StartTime`/`EndTime` are NOT epoch values: the inverter
 * reports a 12h based time, and `AlarmLogParser::getLogEntry()` adds the PM
 * offset plus the local timezone offset -- the result is seconds since local
 * midnight (which is exactly why the Web UI renders it with
 * `timestampToString()` as a bare time of day, `webapp/src/utils/time.ts`).
 *
 * We therefore anchor on TODAY's local midnight; if that lands in the future
 * (beyond the skew tolerance), the event must be from before midnight, so
 * we fall back to yesterday.
 */
static time_t anchorSecondsOfDay(const time_t secondsOfDay, const time_t nowEpoch, const struct tm& nowLocal)
{
    struct tm midnight = nowLocal; // mktime() mutates its argument
    midnight.tm_hour = 0;
    midnight.tm_min = 0;
    midnight.tm_sec = 0;
    midnight.tm_isdst = -1; // let mktime() resolve DST for that local date

    const time_t localMidnight = mktime(&midnight);

    time_t stamp = localMidnight + secondsOfDay;
    if (stamp > nowEpoch + EVENTLOG_FUTURE_TOLERANCE_SEC) {
        stamp -= SECONDS_PER_DAY;
    }
    return stamp;
}

String MqttHandleEventlogClass::toIso8601Utc(const time_t epoch)
{
    struct tm utc;
    gmtime_r(&epoch, &utc);

    char buffer[21] = "";
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return String(buffer);
}

String MqttHandleEventlogClass::buildPayload(std::shared_ptr<InverterAbstract> inv, const time_t nowEpoch, const struct tm& nowLocal)
{
    // Same locale selection as WebApi_eventlog's `locale` parameter, but
    // driven by the configured display locale instead of a query string.
    AlarmMessageLocale_t locale = AlarmMessageLocale_t::EN;
    const String configuredLocale = String(Configuration.get().Display.Locale);
    if (configuredLocale == "de") {
        locale = AlarmMessageLocale_t::DE;
    } else if (configuredLocale == "fr") {
        locale = AlarmMessageLocale_t::FR;
    }

    JsonDocument doc;
    JsonArray events = doc.to<JsonArray>();

    // Clamped: getEntryCount() derives the count from the received payload
    // length, so a malformed/oversized alarm log response could otherwise
    // index past the fixed ALARM_LOG_PAYLOAD_SIZE buffer in getLogEntry().
    const uint8_t entryCount = std::min<uint8_t>(inv->EventLog()->getEntryCount(), ALARM_LOG_ENTRY_COUNT);
    for (uint8_t entryId = 0; entryId < entryCount; entryId++) {
        AlarmLogEntry_t entry;
        inv->EventLog()->getLogEntry(entryId, entry, locale);

        // MessageId 0 is not a Hoymiles event -- it is what an all-zero
        // (not yet filled) slot in the log window decodes to. Publishing it
        // would create a bogus "Unbekanntes Ereignis (Code 0)" downstream.
        if (entry.MessageId == 0) {
            continue;
        }

        const time_t beginn = anchorSecondsOfDay(entry.StartTime, nowEpoch, nowLocal);

        JsonObject event = events.add<JsonObject>();
        event["message_id"] = entry.MessageId;
        event["message"] = entry.Message;
        event["start_time"] = toIso8601Utc(beginn);

        if (entry.EndTime > 0) {
            // Anchor the end on the SAME day as the begin (reuse its
            // midnight), and roll over to the next day if the end time of
            // day is earlier -- that way `ende` is never before `beginn`.
            time_t ende = beginn - entry.StartTime + entry.EndTime;
            if (entry.EndTime < entry.StartTime) {
                ende += SECONDS_PER_DAY;
            }
            event["end_time"] = toIso8601Utc(ende);
        } else {
            // Still active -- explicit null, so a consumer can tell
            // "ongoing" apart from "field missing".
            event["end_time"] = nullptr;
        }
    }

    String payload;
    serializeJson(doc, payload);
    return payload;
}

/** FNV-1a (32 bit) -- change detection only, no cryptographic claim. */
uint32_t MqttHandleEventlogClass::hashPayload(const String& payload)
{
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < payload.length(); i++) {
        hash ^= static_cast<uint8_t>(payload.charAt(i));
        hash *= 16777619u;
    }
    return hash;
}
