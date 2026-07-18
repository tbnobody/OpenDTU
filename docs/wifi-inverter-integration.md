# WiFi Inverter Integration Design Document

**Status:** Implemented
**Date:** 2026-04-25 (design), 2026-04-27 (implementation complete)
**Scope:** Integrating WiFi-based Hoymiles HMS inverter support (from dtuGateway) into OpenDTU

---

## 1. Background

OpenDTU currently communicates with Hoymiles micro-inverters exclusively via RF radio (NRF24 at 2.4 GHz and CMT2300 at ~865 MHz). Newer Hoymiles models in the **HMS-xxxW-2T** series (e.g. HMS-800W-2T) ship with an integrated WiFi DTU rather than relying on a separate RF module. These inverters are not supported by OpenDTU's existing radio stack.

The [dtuGateway](https://github.com/ohAnd/dtuGateway) project has reverse-engineered the proprietary TCP/protobuf protocol these inverters use and provides a working implementation on ESP32/ESP8266. The goal of this integration is to port that communication layer into OpenDTU so that WiFi-based HMS inverters can be configured and monitored through OpenDTU's existing web interface, MQTT stack, and Home Assistant discovery — without maintaining a separate gateway device.

---

## 2. Source Projects

| | OpenDTU | dtuGateway |
|---|---|---|
| **Platform** | ESP32 (PlatformIO) | ESP32 / ESP8266 (Arduino IDE) |
| **License** | GPL-2.0-or-later | Apache 2.0 |
| **Transport** | RF (NRF24 / CMT2300 SPI) | TCP/IP via home WiFi |
| **Protocol** | Reverse-engineered Hoymiles RF | Reverse-engineered Hoymiles TCP+protobuf |
| **Inverters** | HM, HMS (RF), HMT series | HMS-xxxW-2T (WiFi DTU) series |

### License Compatibility

GPL-2.0-or-later is compatible with Apache 2.0 via the GPL v3 upgrade path. The FSF explicitly designed GPL v3 to resolve the incompatibility between GPL v2 and Apache 2.0's patent retaliation clause. Since OpenDTU's SPDX headers already read `GPL-2.0-or-later`, porting Apache 2.0 code from dtuGateway is permitted. The combined work should be distributed under GPL v3 terms.

Obligations when porting dtuGateway code:
- Retain original Apache 2.0 copyright notices in ported files
- No NOTICE file exists in dtuGateway, so no attribution block is required beyond per-file copyright lines

---

## 3. WiFi Inverter Protocol

### 3.1 Network Topology

The Hoymiles HMS WiFi DTU can operate in two network modes:

1. **AP mode** (factory default): The DTU creates its own WiFi access point (`DTUBI-<serial>`). Another device connects to that AP to reach port 10081.
2. **STA mode** (recommended for integration): The DTU joins the home WiFi network as a client. Both OpenDTU and the DTU share the same network. OpenDTU connects to the DTU's home-network IP via TCP port 10081.

For this integration, **STA mode is assumed**. OpenDTU already manages WiFi connectivity; no second WiFi interface or credential set is needed. Only the DTU's IP address (or mDNS hostname) on the shared network is required.

### 3.2 Transport

- **Protocol:** TCP
- **Port:** 10081 (fixed)
- **Client:** OpenDTU initiates the connection
- **Authentication:** None at the application layer — WiFi network access is the security boundary
- **Encryption:** None (plain TCP)

### 3.3 Message Format

Each message is a fixed 10-byte binary header followed by a Protocol Buffer payload:

```
Offset  Length  Description
──────  ──────  ───────────────────────────────────────
0       2       Magic bytes: 0x48 0x4D ("HM")
2       1       Message class: 0xA3 (request) / 0x83 (response)
3       1       Command ID (see table below)
4       2       Fixed: 0x00 0x01
6       2       CRC16 of protobuf payload (big-endian)
8       2       Total message length incl. header (big-endian)
10+     n       Protocol Buffer encoded payload
```

CRC16 is calculated over the protobuf bytes only (not the header).

### 3.4 Command Reference

| Command ID | Direction | Name | Protobuf Message |
|---|---|---|---|
| 0x01 | Req/Resp | App info (device/firmware) | `APPInfoDataResDTO` / `APPInfoDataReqDTO` |
| 0x02 | Req/Resp | Heartbeat / keep-alive | `HBResDTO` |
| 0x04 | Req/Resp | Get alarm data | `WInfoResDTO` / `WInfoReqDTO` |
| 0x05 | Req/Resp | Command (power limit, on/off, request alarms) | `CommandResDTO` |
| 0x09 | Req/Resp | Get config (power limit %, RSSI) | `GetConfigResDTO` / `GetConfigReqDTO` |
| 0x11 | Req/Resp | Real-time data (new format) | `RealDataNewResDTO` / `RealDataNewReqDTO` |
| 0x15 | Req/Resp | Historical power data | `AppGetHistPowerResDTO` |

Command actions within `CommandResDTO` (0x05):

| Action Code | Description |
|---|---|
| 6 | Inverter turn on |
| 7 | Inverter turn off |
| 8 | Set power limit (0–100%) |
| 41 | Grid profile read (not implemented in DTU firmware) |
| 50 | Request alarm list |

**Important:** Alarm fetching uses a two-step flow:
1. **Request alarms** (0xA3/0x05, `CommandResDTO`, action=50) — tells the DTU to prepare alarm data
2. **Get alarms** (0xA3/0x04, `WInfoResDTO`) — fetches the actual alarm entries as `WInfoReqDTO`

The `WInfoReqDTO` contains up to 30 `WInfoMO` entries, each with:
- `WCode`: low byte = alarm message ID (0–255), high bytes = status/flags
- `WTime1`: Unix timestamp — alarm start time
- `WTime2`: Unix timestamp — alarm end time (0 if still active)

### 3.5 Polling Sequence

The client drives a sequential request/response state machine with a 30-second timeout per step:

```
Connect
  └─► App info (0x01)          — firmware versions, inverter model
        └─► [every 31 s]
              Real-time data (0x11)    — DC strings, AC grid, temperature
                └─► Get config (0x09)  — power limit %, DTU RSSI
                      └─► Request alarms (0x05/action=50)  — trigger alarm data prep
                            └─► Get alarms (0x04)          — fetch WInfoReqDTO
                                  └─► Data ready
```

Keep-alive heartbeat is sent every 10 seconds between polling cycles.

The DTU periodically uploads to Hoymiles cloud (~40 s window). During this window responses may time out; the client should tolerate and retry rather than treating this as a hard connection failure.

### 3.6 Data Model

**DC strings (per port, 2 ports on 2T models):**

| Field | Unit | Maps to OpenDTU field |
|---|---|---|
| voltage | V | `FLD_UDC` |
| current | A | `FLD_IDC` |
| power | W | `FLD_PDC` |
| energy_daily | Wh | `FLD_YD` |
| energy_total | kWh | `FLD_YT` |
| error_code | — | (stored separately) |

**AC grid:**

| Field | Unit | Maps to OpenDTU field |
|---|---|---|
| voltage | V | `FLD_UAC` |
| current | A | `FLD_IAC` |
| active_power | W | `FLD_PAC` |
| reactive_power | VAR | `FLD_Q` |
| frequency | Hz | `FLD_F` |
| power_factor | — | `FLD_PF` |
| temperature | °C | `FLD_T` |

**Inverter aggregate:**

| Field | Maps to OpenDTU field |
|---|---|
| daily yield total (pv0+pv1) | `TYPE_INV / FLD_YD` |
| total yield total (pv0+pv1) | `TYPE_INV / FLD_YT` |
| power limit % | `SystemConfigParaParser` |
| warning count | `FLD_EVT_LOG` |

---

## 4. OpenDTU Architecture Summary (Integration Relevant Parts)

### 4.1 Data Flow

```
InverterAbstract
  └── StatisticsParser      ← all channel field values live here
        ↓
  Datastore (1 s)           ← aggregates totals across all inverters
  MqttHandleInverter        ← publishes per-inverter fields to MQTT
  WebApi_ws_live            ← pushes JSON to browser WebSocket
```

Once data is in `StatisticsParser`, the entire downstream pipeline (MQTT, live dashboard, Home Assistant discovery, Datastore totals) works without modification.

### 4.2 Key Interfaces

**`InverterAbstract`** — the contract every inverter must satisfy:

```cpp
// Required overrides
virtual String typeName() const = 0;
virtual const byteAssign_t* getByteAssignment() const = 0;
virtual uint8_t getByteAssignmentSize() const = 0;
virtual bool sendStatsRequest() = 0;
virtual bool sendAlarmLogRequest(bool force) = 0;
virtual bool sendDevInfoRequest() = 0;
virtual bool sendSystemConfigParaRequest() = 0;
virtual bool sendActivePowerControlRequest(float, PowerLimitControlType) = 0;
virtual bool resendActivePowerControlRequest() = 0;
virtual bool sendPowerControlRequest(bool turnOn) = 0;
virtual bool sendRestartControlRequest() = 0;
virtual bool resendPowerControlRequest() = 0;
virtual bool sendGridOnProFileParaRequest() = 0;
virtual bool supportsPowerDistributionLogic() = 0;
```

**`StatisticsParser`** — writable API for injecting data:

```cpp
bool setChannelFieldValue(ChannelType_t, ChannelNum_t, FieldId_t, float value);
void setLastUpdate(uint32_t millis);
void resetRxFailureCount();
void incrementRxFailureCount();
```

**`HoymilesClass::addInverter(name, serial)`** — factory; detects type from serial prefix, constructs and registers the inverter.

**`InverterSettings`** — initialises each configured inverter at boot, applies per-channel config (max power, yield offset), registers the polling task.

### 4.3 Configuration Storage

```cpp
struct INVERTER_CONFIG_T {
    uint64_t Serial;
    char Name[INV_MAX_NAME_STRLEN + 1];
    uint8_t Order;
    bool Poll_Enable, Poll_Enable_Night;
    bool Command_Enable, Command_Enable_Night;
    uint8_t ReachableThreshold;
    bool ZeroRuntimeDataIfUnrechable;
    bool ZeroYieldDayOnMidnight;
    bool ClearEventlogOnMidnight;
    bool YieldDayCorrection;
    CHANNEL_CONFIG_T channel[INV_MAX_CHAN_COUNT];
    char DtuIpAddress[40];  // empty = RF inverter; non-empty = WiFi inverter
};
```

Up to 10 inverters are supported (`INV_MAX_COUNT`).

### 4.4 What the RF Radio Does (and What We Must Replace)

The RF layer is responsible for:
1. Packet serialisation / fragmentation / CRC
2. Queuing commands with retry logic
3. Raising response events back to `InverterAbstract`
4. Providing `isInitialized()` — polled by `Hoymiles::loop()` to gate command dispatch

For a WiFi inverter none of this applies. The replacement is a persistent async TCP connection managed by the inverter class itself.

---

## 5. Integration Design

### 5.1 Configuration Extension

Add one field to `INVERTER_CONFIG_T` in `include/Configuration.h`:

```cpp
char DtuIpAddress[40];   // empty string → RF inverter; set → WiFi inverter
```

- No boolean "IsWifi" flag needed — the presence of a non-empty `DtuIpAddress` is the discriminator.
- Port 10081 is hardcoded (protocol constant).
- `CONFIG_VERSION` must be incremented; a migration entry must be added in `Configuration.migrate()` to zero-initialise the new field on upgrade.

### 5.2 New Library: `lib/HoymilesWifi/`

Self-contained library following the same pattern as `lib/Hoymiles/`:

```
lib/HoymilesWifi/
├── library.json
└── src/
    ├── HmsWifiInverter.h / .cpp     ← InverterAbstract subclass
    ├── DtuTcpClient.h / .cpp        ← TCP + state machine (ported from dtuGateway)
    ├── DtuTcpClient_State.h         ← state enum
    └── proto/                       ← .pb.h/.pb.c files (nanopb, copied from dtuGateway)
        ├── APPInformationData.pb.h
        ├── CommandPB.pb.h
        ├── GetConfig.pb.h
        ├── RealTimeDataNew.pb.h
        └── ...
```

#### 5.2.1 `HmsWifiInverter`

Inherits from `InverterAbstract`. Constructor accepts the DTU IP address string instead of a `HoymilesRadio*` (passes `nullptr` to the parent constructor — the parent stores the pointer but never dereferences it outside of methods we override).

Key method implementations:

| Method | Implementation |
|---|---|
| `sendStatsRequest()` | Triggers the TCP polling cycle if idle; returns true |
| `sendActivePowerControlRequest()` | Sends `CommandResDTO` with action 8 and the limit value |
| `sendPowerControlRequest(on)` | Sends action 6 (on) or 7 (off) |
| `sendAlarmLogRequest()` | Sends action 50 |
| `sendDevInfoRequest()` | Sends command 0x01 |
| `sendSystemConfigParaRequest()` | Piggybacks on next `GetConfig` (0x09) response |
| `sendRestartControlRequest()` | Sends action 3 (MI_REBOOT) |
| `sendGridOnProFileParaRequest()` | No-op, returns true |
| `supportsPowerDistributionLogic()` | Returns false |
| `typeName()` | Returns `"HMS (WiFi)"` |
| `getByteAssignment()` | Returns the `byteAssign_t[]` variant matching the detected string count (1CH/2CH/4CH) |
| `getByteAssignmentSize()` | Returns the size of the selected variant |

**Automatic model detection from serial prefix:**

The constructor calls `_detectPvCount(serial)` which extracts two bytes from the serial prefix: `b0 = (prefix >> 8)`, `b1 = prefix & 0xFF`. Detection uses range-based logic following dtuGateway's convention, covering both WiFi-specific and RF serial ranges:

| `b0` | `b1` range | PV Strings | Models |
|---|---|---|---|
| `0x11` | `0x20–0x29` | 1 | HMS-350W/400W/500-1T |
| `0x11` | `0x40–0x49` | 2 | HMS-600W/700W/800W/1000-2T |
| `0x11` | `0x60–0x69` | 4 | HMS-1000W/1200W/1600/1800-4T |
| `0x10` | any | 2 | HMS-600W-2T variant |
| `0x12` | any | 4 | HMS-1500W-4T |
| `0x14` | `0x00` | 1 | HMS-1CHv2 |
| `0x14` | `0x20–0x29` | 4 | HMS-2000-4T |
| `0x14` | other | 2 | HMS-800W-2T variants |
| `0x28` | any | 2 | SOL-H series |
| (other) | — | 2 | Default fallback |

Three static `byteAssign_t` arrays are defined (`byteAssignment_1CH`, `byteAssignment_2CH`, `byteAssignment_4CH`), each declaring the correct DC channels for that model. The constructor selects the matching array and stores a pointer + size.

`DtuTcpClient` also derives a human-readable model name from the serial using the same prefix, with 21 exact-match entries and range-based fallback for unknown `0x11xx` prefixes.

When `DtuTcpClient` delivers parsed data, `HmsWifiInverter::_onData()`:
- Loops over `min(_pvCount, d.pvCount)` DC strings, setting field values dynamically
- Sums `totalDcPower` across all active strings
- Calls `Statistics()->setLastUpdate(millis())`
- Calls `Statistics()->resetRxFailureCount()` on successful data
- Calls `Statistics()->incrementRxFailureCount()` on timeout/parse failure

#### 5.2.2 `DtuTcpClient`

Ported from `dtuGateway/src/dtuInterface.cpp`. Key adaptations:

- Uses `AsyncClient` (async_tcp) — already available as a transitive dependency of `ESPAsyncWebServer`.
- `DtuData_t` replaces dtuGateway's global struct; delivered to `HmsWifiInverter` via callback.
- `DtuData_t.pv[4]` supports up to 4 DC strings (MAX_PV_COUNT = 4); `pvCount` tracks how many the DTU actually returned.
- RealData parse loop iterates up to `pvCount` (not hardcoded); energy aggregation sums all active strings.
- Replace dtuGateway's `Ticker` timers with `TaskScheduler` tasks (already used by OpenDTU) for keep-alive and polling intervals.
- Remove cloud-pause logic initially (can be added later as a config option).
- Remove display, MQTT, OpenHAB code — all irrelevant here.

State machine states to preserve:

```cpp
enum DtuTcpState {
    OFFLINE,
    CONNECTING,
    CONNECTED,
    CONNECT_ERROR,
};

enum DtuTxRxState {
    IDLE,
    WAIT_APP_INFO,
    WAIT_REALDATANEW,
    WAIT_GETCONFIG,
    WAIT_REQUEST_ALARMS,
    WAIT_GET_ALARMS,
};
```

### 5.3 Factory & Hoymiles Loop Changes

#### `Hoymiles.cpp` — `addInverter()`

Currently the factory only creates inverter objects. We need a variant that accepts a DTU IP:

```cpp
// New overload in HoymilesClass:
std::shared_ptr<InverterAbstract> addWifiInverter(
    const char* name, const uint64_t serial, const char* dtuIp);
```

This constructs `HmsWifiInverter`, calls `init()`, and pushes to `_inverters`. The existing `addInverter()` is unchanged.

#### `Hoymiles.cpp` — `loop()`

The existing loop sends RF commands to each inverter. For WiFi inverters the TCP client manages its own polling, so the loop must skip RF command dispatch:

```cpp
if (!iv->isWifiInverter()) {
    iv->sendStatsRequest();
    iv->sendAlarmLogRequest(force);
    // ... rest of RF command dispatch
}
```

`isWifiInverter()` is a non-virtual method on `InverterAbstract` returning false by default; `HmsWifiInverter` overrides it to return true.

The `Hoymiles::isAllRadioIdle()` check (used by `Datastore` and `MqttHandleInverter`) should also return true if all non-WiFi inverters are idle — WiFi inverters do not affect radio idle state.

### 5.4 `InverterSettings` Changes

In `InverterSettings::init()`, after the existing loop that calls `Hoymiles.addInverter()`:

```cpp
for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
    const auto& inv_cfg = config.Inverter[i];
    if (inv_cfg.Serial == 0) continue;

    std::shared_ptr<InverterAbstract> inv;

    if (strlen(inv_cfg.DtuIpAddress) > 0) {
        inv = Hoymiles.addWifiInverter(inv_cfg.Name, inv_cfg.Serial, inv_cfg.DtuIpAddress);
    } else {
        inv = Hoymiles.addInverter(inv_cfg.Name, inv_cfg.Serial);
    }

    if (inv == nullptr) { /* log warning */ continue; }

    // Apply common config (unchanged):
    inv->setReachableThreshold(inv_cfg.ReachableThreshold);
    // ...
}
```

### 5.5 Serial Number Handling

Users enter the inverter's real Hoymiles serial number. The serial prefix (`(serial >> 32) & 0xFFFF`) determines the model variant (1T/2T/4T) using range-based detection derived from dtuGateway's `getInverterModelFromIntSerial()`. This covers both WiFi-specific models (HMS-xxxW) and RF models, as they share the same serial prefix scheme. See §5.2.1 for the full detection table.

The WiFi path is triggered by a non-empty `DtuIpAddress` in the config — the serial prefix is only used for model detection, not for choosing RF vs WiFi transport.

### 5.6 Web API Changes

#### `WebApi_inverter.cpp`

The `/api/inverter/add` and `/api/inverter/edit` endpoints currently accept:

```json
{ "serial": "...", "name": "..." }
```

Extend to optionally accept:

```json
{ "serial": "...", "name": "...", "dtu_ip": "192.168.1.100" }
```

If `dtu_ip` is provided and non-empty, it is stored in `INVERTER_CONFIG_T.DtuIpAddress`.

The `/api/inverter/list` response similarly gains a `"dtu_ip"` field.

No new endpoints are required — WiFi inverters appear in the standard inverter list and their live data flows through the existing WebSocket and MQTT paths.

### 5.7 Vue.js Frontend Changes

The inverter add/edit form (in `webapp/src/`) needs:
- An optional "DTU IP Address" text field
- Help text explaining it should be left blank for RF inverters
- Validation: either empty or a valid IPv4 address / hostname

This is a small, self-contained UI change in the existing inverter settings component.

---

## 6. What Requires No Changes

The following subsystems work unchanged once data is in `StatisticsParser`:

| Subsystem | File | Reason |
|---|---|---|
| MQTT inverter publishing | `MqttHandleInverter.cpp` | Iterates `Hoymiles._inverters`, reads `Statistics()` |
| MQTT total publishing | `MqttHandleInverterTotal.cpp` | Reads from `Datastore` |
| Home Assistant discovery | `MqttHandleHass.cpp` | Uses same inverter list |
| Datastore aggregation | `Datastore.cpp` | Iterates `Hoymiles._inverters` |
| Power limit commands | `WebApi_inverter.cpp` | Calls `sendActivePowerControlRequest()` — overridden |
| On/off commands | Same | Calls `sendPowerControlRequest()` — overridden |
| Reachability status | `InverterAbstract` | Derived from `getRxFailureCount()` — we control this |
| Daily yield zeroing | `InverterAbstract` | Calls `Statistics()->zeroDailyData()` — works unchanged |

### 6.1 Subsystems With WiFi-Specific Adaptations

| Subsystem | File | Adaptation |
|---|---|---|
| Live dashboard WebSocket | `WebApi_ws_live.cpp` | Added `wifi_inverter` flag; skip `radio_stats` for WiFi inverters |
| Live dashboard UI | `HomeView.vue` | Hide "Radio statistics" accordion for WiFi inverters |
| Event log / alarms | `AlarmLogParser.h/.cpp` | Added `setDirectEntries()`/`clearDirectEntries()` for WiFi alarm format (bypasses RF byte buffer) |
| Grid profile API | `WebApi_gridprofile.cpp` | Added `wifi_inverter` flag to status response |
| Grid profile UI | `GridProfile.vue` | Shows "Not available for WiFi inverters" instead of generic "No info" |

---

## 7. Implementation Phases

### Phase 1 — Core TCP Communication ✓
- [x] Copy protobuf files from dtuGateway into `lib/HoymilesWifi/src/proto/`
- [x] Port `DtuTcpClient` from `dtuInterface.cpp`, adapted to async_tcp
- [x] Connect to a live DTU, receive real-time data

### Phase 2 — Inverter Class ✓
- [x] Implement `HmsWifiInverter` extending `InverterAbstract`
- [x] Define `byteAssign_t` arrays for 1-string, 2-string, and 4-string models
- [x] Auto-detect model variant from serial prefix (same logic as RF HMS_1CH/2CH/4CH)
- [x] Map `DtuTcpClient` callbacks to `Statistics()->setChannelFieldValue()` calls
- [x] Implement power limit and on/off command dispatch
- [x] Populate `DevInfo` (max power, firmware version, hardware version) from AppInfo response
- [x] Populate alarms via two-step flow (request alarms → get alarms)

### Phase 3 — OpenDTU Wiring ✓
- [x] Add `DtuIpAddress` field to `INVERTER_CONFIG_T`, increment `CONFIG_VERSION`
- [x] Add `registerInverter()` to `HoymilesClass`
- [x] Update `InverterSettings::init()` to call the right factory
- [x] Add `isWifiInverter()` guard in `Hoymiles::loop()` and `removeInverterBySerial()`

### Phase 4 — Web UI ✓
- [x] Extend `/api/inverter/add` and `/api/inverter/edit` to accept `dtu_ip`
- [x] Update `/api/inverter/list` to return `dtu_ip`
- [x] Add DTU IP field and WiFi inverter toggle to the Vue.js inverter settings form
- [x] Hide "Radio statistics" on dashboard for WiFi inverters
- [x] Show "Not available for WiFi inverters" for grid profile page
- [x] Add `wifi_inverter` flag to live data WebSocket response

### Phase 5 — Integration Testing (partial)
- [x] Add a WiFi inverter via the web UI
- [x] Verify live data appears on dashboard
- [x] Verify alarm/event log entries appear correctly
- [x] Verify MQTT topics published correctly
- [x] Verify power limit command reaches the inverter
- [x] Verify on/off command reaches the inverter
- [x] Verify reachability state changes on DTU disconnect
- [x] Verify RF inverters still work alongside WiFi inverters

---

## 8. Resolved Questions

1. **Serial number strategy:** Users enter the inverter's real serial number manually. The DTU's own serial is separate and received in the AppInfo response but not used for identification.

2. **Cloud-pause handling:** Implemented as timeout/retry logic in `DtuTcpClient`. The DTU periodically goes offline for cloud sync; the client tolerates this and reconnects automatically.

3. **Grid profile:** Investigated via protocol testing (`tools/dtu_grid_profile_test.py`). The DTU acks a grid profile read request (action=41) without error, but never returns profile data. This appears to be a cloud-only feature. The UI shows "Not available for WiFi inverters" instead.

4. **Alarm format:** WiFi alarms (`WInfoMO`) use a different format than RF alarms (12-byte entries). The `AlarmLogParser` was extended with a direct-entry API (`setDirectEntries()`) that bypasses the RF byte buffer format. Timestamps are Unix timestamps with timezone offset applied.

## 9. Remaining Open Questions

1. **mDNS support:** `DtuIpAddress` currently accepts only IP addresses. Hostname/mDNS resolution could be added later.

---

## 10. Implementation Notes

### 10.1 async_tcp Stack Constraints

`DtuTcpClient` runs within async_tcp's event callbacks, which have a limited stack (~4KB). The `WInfoReqDTO` struct (~1KB, contains 30 × `WInfoMO`) must be heap-allocated via `malloc`/`free` rather than placed on the stack.

nanopb's `_init_default` macros are brace-enclosed initializer lists and cannot be used in assignment (`*ptr = FooDTO_init_default` fails). Use stack initialization + `memcpy` instead:

```cpp
auto* resp = static_cast<WInfoReqDTO*>(malloc(sizeof(WInfoReqDTO)));
WInfoReqDTO init = WInfoReqDTO_init_default;
memcpy(resp, &init, sizeof(WInfoReqDTO));
// ... use resp ...
free(resp);
```

### 10.2 StatisticsParser Unsigned Underflow

`StatisticsParser::getChannelFieldValue()` returns `0` for fields that have never been set, but `setChannelFieldValue()` performs delta calculations. Care was taken to ensure the WiFi path only sets fields that have actual data from the DTU response.

### 10.3 Alarm Timezone Handling

Both RF and WiFi paths pre-add the timezone offset to alarm timestamps before storing them. The frontend displays these as UTC (`timeZone: 'UTC'` in `toLocaleTimeString()`), which effectively shows local time. The WiFi path uses `AlarmLogParser::getTimezoneOffset()` (made public for this purpose) to apply the same offset.

### 10.4 DTU Cloud Sync

The DTU periodically uploads to Hoymiles cloud (~40s window). During this window, TCP responses may time out. `DtuTcpClient` handles this gracefully with reconnect logic rather than treating it as a permanent failure.

### 10.5 Grid Profile Unavailability

Protocol testing confirmed that the DTU firmware (tested: 2.1.21.4_hm) acknowledges grid profile read requests (action=41) without error but never returns data. This is a cloud-only feature. The frontend shows a clear message rather than a confusing "no data" state.

---

## 11. File Change Summary

### New files

| File | Description |
|---|---|
| `lib/HoymilesWifi/library.json` | PlatformIO library manifest |
| `lib/HoymilesWifi/src/DtuTcpClient.h` | TCP client header — state machine, DtuData_t, DtuAlarmEntry_t |
| `lib/HoymilesWifi/src/DtuTcpClient.cpp` | TCP client — async_tcp, protobuf encode/decode, alarm two-step flow |
| `lib/HoymilesWifi/src/HmsWifiInverter.h` | WiFi inverter class header |
| `lib/HoymilesWifi/src/HmsWifiInverter.cpp` | WiFi inverter — InverterAbstract subclass, data population, alarm population |
| `lib/HoymilesWifi/src/proto/*.proto` | Protobuf definitions (copied from dtuGateway) |
| `lib/HoymilesWifi/src/proto/*.pb.h` / `*.pb.c` | nanopb generated protobuf code |
| `tools/dtu_grid_profile_test.py` | Python script for DTU protocol exploration |
| `tools/dtu_wifi_credentials.py` | Python script to extract WiFi SSID/password from DTU |

### Modified files

| File | Change |
|---|---|
| `include/Configuration.h` | Add `DtuIpAddress[40]` to `INVERTER_CONFIG_T`; bump `CONFIG_VERSION` to `0x00011f00` |
| `src/Configuration.cpp` | Add `dtu_ip` JSON read/write |
| `lib/Hoymiles/src/Hoymiles.h` | Declare `registerInverter()` |
| `lib/Hoymiles/src/Hoymiles.cpp` | Implement `registerInverter()`; guard `loop()` and `removeInverterBySerial()` for WiFi inverters |
| `lib/Hoymiles/src/inverters/InverterAbstract.h` | Add `virtual bool isWifiInverter() const { return false; }` |
| `lib/Hoymiles/src/parser/AlarmLogParser.h` | Add `setDirectEntries()`/`clearDirectEntries()`, make `getTimezoneOffset()` public |
| `lib/Hoymiles/src/parser/AlarmLogParser.cpp` | Implement direct entry API; modify `getEntryCount()`/`getLogEntry()` for WiFi path |
| `src/InverterSettings.cpp` | Remove early RF-only guard; add WiFi branch using `registerInverter()` |
| `src/WebApi_inverter.cpp` | Add `dtu_ip` to list/add/edit endpoints; WiFi path for add/edit |
| `src/WebApi_ws_live.cpp` | Add `wifi_inverter` flag; skip `radio_stats` for WiFi inverters |
| `src/WebApi_gridprofile.cpp` | Add `wifi_inverter` boolean to status response |
| `webapp/src/views/HomeView.vue` | Hide "Radio statistics" accordion for WiFi inverters |
| `webapp/src/views/InverterAdminView.vue` | Add WiFi inverter toggle and DTU IP field |
| `webapp/src/components/GridProfile.vue` | Show WiFi-specific "not supported" alert |
| `webapp/src/types/LiveDataStatus.ts` | Add `wifi_inverter: boolean` to `Inverter` interface |
| `webapp/src/types/GridProfileStatus.ts` | Add `wifi_inverter: boolean` |
| `webapp/src/locales/en.json` | Add i18n strings for WiFi inverter UI elements |
| `platformio.ini` | Add `nanopb/Nanopb @^0.4.8` and `custom_nanopb_protos` |
