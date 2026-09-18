// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Grid power source: Shelly Local Network Messaging (LNM).
 *
 * Joins the configured UDP multicast group and keeps the most recent grid
 * active power value reported by Shelly devices (energy meters, plugs, ...)
 * using their LNM status messages.
 *
 * Wire protocol and JSON payload are documented at:
 * - https://shelly-api-docs.shelly.cloud/gen2/General/LocalNetworkMessaging/
 * - https://shelly-api-docs.shelly.cloud/gen2/DynamicComponents/LNM/
 */
#include "Configuration.h"
#include "NetworkSettings.h"
#include "ZeroExport_ShellyLNM.h"
#include <ArduinoJson.h>

#undef TAG
static const char* TAG = "zeroexport";

ZeroExportShellyLnmClass ZeroExportShellyLnm;

ZeroExportShellyLnmClass::ZeroExportShellyLnmClass() = default;

void ZeroExportShellyLnmClass::start()
{
    const CONFIG_T& config = Configuration.get();
    setPowerTimeoutMs(config.ZeroExport.ShellyLnm.FailsafeTimeout * 1000UL);
    const char* groupAddr = config.ZeroExport.ShellyLnm.GroupAddress;
    const uint16_t groupPort = config.ZeroExport.ShellyLnm.GroupPort;
    _type = config.ZeroExport.ShellyLnm.Type;

    // Idempotent: the controller calls start() whenever isRunning() is false,
    // which also happens while the network is down even though this UDP
    // listener stays registered. Re-joining the same multicast group would
    // be a silent but useless operation.
    if (_listening && strcmp(_listenAddr, groupAddr) == 0 && _listenPort == groupPort) {
        return;
    }

    stop();

    if (!NetworkSettings.isConnected()) {
        return;
    }

    IPAddress addr;
    if (!addr.fromString(groupAddr)) {
        ESP_LOGE(TAG, "Invalid Shelly LNM multicast address: %s", groupAddr);
        return;
    }

    if (!_udp.beginMulticast(addr, groupPort)) {
        ESP_LOGE(TAG, "Failed to start Shelly LNM listener on %s:%u", groupAddr, groupPort);
        return;
    }

    strlcpy(_listenAddr, groupAddr, sizeof(_listenAddr));
    _listenPort = groupPort;
    _listening = true;

    ESP_LOGI(TAG, "Shelly LNM listener started on %s:%u", _listenAddr, _listenPort);
}

void ZeroExportShellyLnmClass::stop()
{
    if (_listening) {
        _udp.stop();
        _listening = false;
        ESP_LOGI(TAG, "Shelly LNM listener stopped");
    }
    // Do not let a quick disable/re-enable reuse a measurement from the
    // previous listener session. The new session must receive a fresh packet.
    clearPower();
}

bool ZeroExportShellyLnmClass::isRunning() const
{
    return _listening;
}

void ZeroExportShellyLnmClass::loop()
{
    if (!_listening) {
        return;
    }

    // Process all queued packets, keep only the most recent grid value
    int packetSize;
    while ((packetSize = _udp.parsePacket()) > 0) {
        static uint8_t buffer[1460];
        if (packetSize > static_cast<int>(sizeof(buffer))) {
            _udp.flush();
            ESP_LOGW(TAG, "Shelly LNM packet too large: %d", packetSize);
            continue;
        }

        const int len = _udp.read(buffer, sizeof(buffer));
        if (len > 0) {
            processPacket(buffer, static_cast<size_t>(len));
        }
    }
}

void ZeroExportShellyLnmClass::processPacket(const uint8_t* data, size_t len)
{
    // Shelly LNM wire protocol:
    //   [header (12 bytes)] [payload (payload_len bytes)] [meta (meta_len bytes)]
    //
    // Binary header (little-endian, packed):
    //   offset 0  size 2  magic        0x53 0x4C ("SL")
    //   offset 2  size 1  version      protocol version (currently 0)
    //   offset 3  size 1  payload_type 1 = status/event
    //   offset 4  size 2  payload_len  payload length in bytes (LE)
    //   offset 6  size 2  meta_len     meta block length (currently 0)
    //   offset 8  size 4  reserved
    //
    // For payload_type 1 the payload is a JSON object:
    // {
    //   "device": "shellyproem50-...",
    //   "status": {
    //     "em1:0": { "id": 0, "voltage": 241.8, "current": 3.559, "act_power": 291.5 }
    //   }
    // }
    // {
    //   "device": "shellypro3em-8813bfe1ab68",
    //   "ts": 1790183354.65,
    //   "status": {
    //     "em:0": {
    //       "id": 0,
    //       "a_current": 0.027,
    //       "a_voltage": 236.2,
    //       "a_act_power": 0.0,
    //       "a_aprt_power": 6.5,
    //       "a_pf": 0.00,
    //       "a_freq": 50.0,
    //       "b_current": 0.182,
    //       "b_voltage": 236.2,
    //       "b_act_power": 20.9,
    //       "b_aprt_power": 43.0,
    //       "b_pf": 0.45,
    //       "b_freq": 50.0,
    //       "c_current": 1.301,
    //       "c_voltage": 236.2,
    //       "c_act_power": 165.5,
    //       "c_aprt_power": 307.4,
    //       "c_pf": 0.54,
    //       "c_freq": 50.0,
    //       "n_current": null,
    //       "total_current": 1.511,
    //       "total_act_power": 186.310,
    //       "total_aprt_power": 356.925,
    //       "user_calibrated_phase": []
    //     }
    //   }
    // }

    if (len < 12) {
        ESP_LOGW(TAG, "LNM packet too short: %u", len);
        return;
    }

    if (data[0] != 0x53 || data[1] != 0x4C) {
        ESP_LOGW(TAG, "LNM: bad magic, not a Shelly packet");
        return; // Not a Shelly LNM message
    }

    const uint8_t payloadType = data[3];
    const uint16_t payloadLen = data[4] | (data[5] << 8);
    const uint16_t metaLen = data[6] | (data[7] << 8);

    if (payloadType != 1) {
        ESP_LOGD(TAG, "LNM: payload type %u, not status", payloadType);
        return; // Only status/event payloads carry the JSON we need
    }

    if (12u + payloadLen + metaLen > len || payloadLen == 0) {
        ESP_LOGW(TAG, "LNM truncated/empty packet: payload=%u meta=%u len=%u", payloadLen, metaLen, len);
        return;
    }

    JsonDocument doc;
    const DeserializationError err = deserializeJson(doc, reinterpret_cast<const char*>(data) + 12, payloadLen);
    if (err) {
        ESP_LOGW(TAG, "LNM JSON parse error: %s (payload_len=%u)", err.c_str(), payloadLen);
        return;
    }

    ESP_LOGD(TAG, "Shelly LNM JSON payload: %.*s", static_cast<int>(payloadLen), reinterpret_cast<const char*>(data) + 12);

    JsonObject status = doc["status"].as<JsonObject>();
    if (status.isNull()) {
        ESP_LOGW(TAG, "LNM: no 'status' object in payload");
        return;
    }

    bool found = false;
    for (JsonPair kv : status) {
        JsonObject component = kv.value().as<JsonObject>();
        if (component.isNull()) {
            continue;
        }

        if (!component[_type].is<float>()) {
            continue;
        }

        float p = component[_type].as<float>();
        setPower(p);
        ESP_LOGI(TAG, "Grid Power from Shelly: %.1f W", p);
        found = true;
        break; // Use the first component with a power value
    }
    if (!found) {
        ESP_LOGW(TAG, "LNM: no component found with '%s'", _type.c_str());
    }
}
