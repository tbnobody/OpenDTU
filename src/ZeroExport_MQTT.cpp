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
#include "ZeroExport.h"
#include "Configuration.h"
#include "MqttHandleHass.h"
#include "MqttSettings.h"
#include <charconv>

#undef TAG
static const char* TAG = "zeroexport";

void ZeroExportClass::publishStatus()
{
    if (!MqttSettings.getConnected() || millis() - _lastStatusPublish < Configuration.get().Mqtt.PublishInterval * TASK_SECOND) {
        return;
    }

    _lastStatusPublish = millis();

    MqttSettings.publish("dtu/zeroexport/status/enabled", String(isEnabled()));
    MqttSettings.publish("dtu/zeroexport/status/setpoint", String(getSetPoint()));
    MqttSettings.publish("dtu/zeroexport/status/production_limit", String(getProductionLimit(), 0));
    MqttSettings.publish("dtu/zeroexport/status/failsafe", String(isGridFailSafeActive()));
    // Keep this in seconds to match the Zero-Export status API. It is useful
    // for distinguishing an intermittent source from a source that stopped
    // publishing altogether before the fail-safe threshold is reached.
    MqttSettings.publish("dtu/zeroexport/status/grid_power_age", String(getGridPowerAgeMs() / 1000UL));
    // EWMA of the source's sample interval: 0 means fewer than two samples
    // have been received since the source (re)started.
    MqttSettings.publish("dtu/zeroexport/status/grid_update_period", String(getSourceUpdatePeriodMs()));
}

void ZeroExportClass::subscribeTopics()
{
    const String prefix = MqttSettings.getPrefix();
    const String enabledTopic = prefix + "dtu/zeroexport/cmd/enabled";
    const String setPointTopic = prefix + "dtu/zeroexport/cmd/setpoint";

    MqttSettings.unsubscribe(enabledTopic);
    MqttSettings.unsubscribe(setPointTopic);

    ESP_LOGI(TAG, "Subscribe to Zero-Export MQTT topics: %s, %s",
        enabledTopic.c_str(), setPointTopic.c_str());

    MqttSettings.subscribe(enabledTopic, 0,
        std::bind(&ZeroExportClass::onMqttMessageEnabled, this,
            std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3, std::placeholders::_4));

    MqttSettings.subscribe(setPointTopic, 0,
        std::bind(&ZeroExportClass::onMqttMessageSetPoint, this,
            std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3, std::placeholders::_4));
}

void ZeroExportClass::onMqttMessageEnabled(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, const size_t len)
{
    // This runs in the MQTT client's own task. It must not take the
    // configuration write guard (which blocks this task until the main loop
    // grants it) nor call applyConfig() (which publishes and subscribes on
    // the same MQTT client): either can deadlock the whole system when the
    // main loop in turn waits for the MQTT client. The parsed value is only
    // recorded here; the Zero-Export loop applies it from the main-loop task.
    ESP_LOGD(TAG, "Received Zero-Export enabled command on %s", topic);
    const std::string value(reinterpret_cast<const char*>(payload), len);
    const bool enable = (value == "true" || value == "on" || value == "1" || value == "ON");
    if (_enabled != enable)
    {
        _enabled = enable;
        ESP_LOGI(TAG, "Zero-Export %s via MQTT", enable ? "enabled" : "disabled");
        _lastStatusPublish = 0;
        requestApplyConfig();
    }
}

void ZeroExportClass::onMqttMessageSetPoint(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, const size_t len)
{
    // Runs in the MQTT client task: only parse, validate and record (see
    // onMqttMessageEnabled). The Zero-Export loop applies the change.
    ESP_LOGD(TAG, "Received Zero-Export setpoint command on %s", topic);
    const char* begin = reinterpret_cast<const char*>(payload);
    int32_t value = 0;
    const auto [ptr, ec] = std::from_chars(begin, begin + len, value);
    if (ec != std::errc{}) {
        ESP_LOGW(TAG, "Invalid setpoint payload");
        return;
    }
    const int16_t sanitized = clampSetPoint(value);
    if (_setPoint != sanitized)
    {
        _setPoint = sanitized;
        // A setpoint change alone does not restart the grid source.
        _lastStatusPublish = 0;
        ESP_LOGI(TAG, "Zero-Export setpoint set to %" PRId16 " W via MQTT", sanitized);
    }
}
