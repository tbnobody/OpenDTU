# opendtu-lokalkraft

> **Fork of [tbnobody/OpenDTU](https://github.com/tbnobody/OpenDTU).** The
> upstream README follows unchanged below.

## Why this fork exists

lokalKraft's "Puls" monitoring pipeline (Mosquitto → collector →
VictoriaMetrics/SQLite → member portal) shows per-inverter events
(*Wechselrichter gestartet*, *PV-1: Kein Eingang*, …). Upstream OpenDTU
exposes the inverter event log **only** over the HTTP Web API
(`/api/eventlog/status`), which requires polling every DTU and holding
credentials for each of them — not workable for member-owned devices.

This fork adds exactly one thing: the event log is **also** published over
MQTT, so a DTU that is already publishing to the lokalKraft broker delivers
its events without any extra channel.

### What the patch does

* New MQTT setting **`Mqtt.EventlogEnabled`** ("Ereignislog veröffentlichen" /
  "Publish event log") on the MQTT settings page — **opt-in, default off**.
  With the switch off the firmware behaves exactly like upstream OpenDTU.
* With the switch on, every inverter publishes

  ```
  [base-topic][inverter-serial]/eventlog
  ```

  as a **retained** JSON array:

  ```json
  [
    { "code": 1,   "meldung": "Wechselrichter gestartet", "beginn": "2026-09-03T05:12:44Z", "ende": "2026-09-03T05:12:44Z" },
    { "code": 212, "meldung": "PV-4: Kein Eingang",       "beginn": "2026-09-03T11:03:10Z", "ende": null }
  ]
  ```

  * `code` — the Hoymiles `message_id` (same value the Web API returns).
  * `meldung` — the localized message text, locale taken from the configured
    display locale (`de`/`fr`/else English), same table as the Web UI.
  * `beginn` / `ende` — real ISO-8601 UTC timestamps. `ende` is `null` while
    the event is still active.
  * Published **only on change** (payload hash per inverter), not on every
    publish interval, and forced retained so a consumer that connects later
    still sees the currently active faults.
* Source of the data is the very same datastore `WebApi_eventlog` serializes
  (`inv->EventLog()`); this fork does not add any inverter polling.

### About the timestamps

The raw alarm log does **not** contain dates. `AlarmLogParser` yields a
*time of day* (seconds since local midnight; that is why the Web UI renders
`start_time`/`end_time` as a bare clock time). This fork anchors that time of
day on the DTU's NTP-synchronized date: today's local midnight, or yesterday's
if that would put the event in the future. **Without a valid NTP time nothing
is published** — a wrong timestamp is worse than none.

Consequence: an event older than ~24 h is anchored on the wrong day. In
practice the alarm log holds at most 15 entries and is refreshed
continuously, so this only affects long-standing, still-active faults.

**Downstream effect of that limitation.** For a fault that stays active
across midnight, `beginn` is re-anchored to the new day on every publish
after midnight. A consumer that keys events by their start time — the
lokalKraft collector does, its `puls_ereignisse` table has
`UNIQUE(geraet_id, wr_serial, quelle, code, beginn)` — therefore records a
**new row per day**, and each of those rows still has `ende = null`. A fault
running for three days shows up as three separate "active" entries instead
of one. Collapsing them is a consumer-side job (a follow-up ticket on the
lokalKraft side, not part of this firmware patch): the firmware genuinely
does not know the real start date, so it cannot emit a stable key for it.

Entries whose `message_id` is 0 are skipped — that is what an unfilled slot
in the log window decodes to, not a real Hoymiles event.

### Files touched by the patch

```
include/Configuration.h        + Mqtt.EventlogEnabled
include/defaults.h             + MQTT_EVENTLOG_ENABLED (false)
src/Configuration.cpp          + config.json read/write
src/WebApi_mqtt.cpp            + /api/mqtt/status + /api/mqtt/config (GET/POST)
include/MqttHandleEventlog.h   NEW
src/MqttHandleEventlog.cpp     NEW
src/main.cpp                   + MqttHandleEventlog.init(scheduler)
webapp/src/types/MqttConfig.ts,
webapp/src/types/MqttStatus.ts,
webapp/src/views/MqttAdminView.vue,
webapp/src/views/MqttInfoView.vue,
webapp/src/locales/{de,en,fr}.json
.gitlab-ci.yml                 NEW (fork-only build pipeline)
```

## Rebase strategy: exactly one commit on top

The `lokalkraft` branch is deliberately kept as **a single patch commit on
top of an upstream release tag** — never a merge, never a series. That keeps
the delta reviewable and the rebase mechanical.

Current base: **`v26.3.30`** (commit `b6f0353b`).

```bash
git remote add upstream https://github.com/tbnobody/OpenDTU.git   # once
git fetch upstream --tags

git checkout lokalkraft
git rebase <new-upstream-tag>        # e.g. v26.4.x
# resolve conflicts (expected hotspots: Configuration.cpp, WebApi_mqtt.cpp,
# MqttAdminView.vue -- all of them single-line insertions next to the
# neighbouring MQTT switches)
git push --force-with-lease lokalkraft lokalkraft
```

If the rebase produces more than one commit (because a conflict resolution
was committed separately), squash it back down before pushing:

```bash
git reset --soft <new-upstream-tag> && git commit -C lokalkraft@{1}
```

## Building

CI (`.gitlab-ci.yml`) builds the web app first and then one firmware per
environment (`generic`, `generic_esp32`, `generic_esp32s3`,
`generic_esp32s3_usb`); the `.bin` files are job artifacts.

Locally:

```bash
cd webapp && yarn install --frozen-lockfile && yarn build && cd ..
pio run -e generic_esp32
```

The web app build is **not** optional when you change the UI: the firmware
embeds `webapp_dist/`, which is committed in git. A bare `pio run` compiles
fine but ships the committed (upstream) web UI.

## Flashing / OTA and fallback

* Update path is the normal OpenDTU one: DTU web UI → *Firmware Upgrade* →
  upload `opendtu-lokalkraft-<env>-<version>.bin` (the **non**-factory file).
  The OTA partition layout is unchanged from upstream.
* **Fallback:** because the partition table is untouched, flashing an
  official upstream release over this fork works the same way and restores
  stock behaviour. The `mqtt.eventlog_enabled` key left behind in
  `config.json` is simply ignored by upstream firmware.
* Only use `*.factory.bin` (serial flash at offset 0x0) for a first
  installation or to recover a bricked device — it wipes the configuration.
* Flashing is a manual, per-device step; this repository never flashes
  anything.

## Upstream contribution

An upstream pull request to `tbnobody/OpenDTU` is **planned as a separate,
explicitly approved step** and has *not* been opened. Nothing in this
repository is pushed to github.com.

## License

OpenDTU is licensed **GPL-2.0-or-later**; this fork and every change in it
are published under the same license. See [LICENSE](LICENSE) / [COPYING](COPYING).
Copyright of the upstream code remains with Thomas Basler and the OpenDTU
contributors.

---

# OpenDTU

[![OpenDTU Build](https://github.com/tbnobody/OpenDTU/actions/workflows/build.yml/badge.svg)](https://github.com/tbnobody/OpenDTU/actions/workflows/build.yml)
[![cpplint](https://github.com/tbnobody/OpenDTU/actions/workflows/cpplint.yml/badge.svg)](https://github.com/tbnobody/OpenDTU/actions/workflows/cpplint.yml)
[![Yarn Linting](https://github.com/tbnobody/OpenDTU/actions/workflows/yarnlint.yml/badge.svg)](https://github.com/tbnobody/OpenDTU/actions/workflows/yarnlint.yml)
[![Yarn Prettier](https://github.com/tbnobody/OpenDTU/actions/workflows/yarnprettier.yml/badge.svg)](https://github.com/tbnobody/OpenDTU/actions/workflows/yarnprettier.yml)

## !! IMPORTANT UPGRADE NOTES !!

If you are upgrading from a version before 15.03.2023 you have to upgrade the partition table of the ESP32. Please follow the [this](docs/UpgradePartition.md) documentation!

## Background

This project was started from [this](https://www.mikrocontroller.net/topic/525778) discussion (Mikrocontroller.net).
It was the goal to replace the original Hoymiles DTU (Telemetry Gateway) with their cloud access. With a lot of reverse engineering the Hoymiles protocol was decrypted and analyzed.

## Documentation

The documentation can be found [here](https://tbnobody.github.io/OpenDTU-docs/).
Please feel free to support and create a PR in [this](https://github.com/tbnobody/OpenDTU-docs) repository to make the documentation even better.

## Breaking changes

Generated using: `git log --date=short --pretty=format:"* %h%x09%ad%x09%s" | grep BREAKING`

```code
* 8cab3335      2025-08-07      BREAKING CHANGE: WebAPI endpoint `/api/limit/config` requires different parameters
* 8372deaf      2025-04-18      BREAKING CHANGE: Logging newline changed from "\r\n" to "\n"
* 1b637f08      2024-01-30      BREAKING CHANGE: Web API Endpoint /api/livedata/status and /api/prometheus/metrics
* e1564780      2024-01-30      BREAKING CHANGE: Web API Endpoint /api/livedata/status and /api/prometheus/metrics
* f0b5542c      2024-01-30      BREAKING CHANGE: Web API Endpoint /api/livedata/status and /api/prometheus/metrics
* c27ecc36      2024-01-29      BREAKING CHANGE: Web API Endpoint /api/livedata/status
* 71d1b3b       2023-11-07      BREAKING CHANGE: Home Assistant Auto Discovery to new naming scheme
* 04f62e0       2023-04-20      BREAKING CHANGE: Web API Endpoint /api/eventlog/status no nested serial object
* 59f43a8       2023-04-17      BREAKING CHANGE: Web API Endpoint /api/devinfo/status requires GET parameter inv=
* 318136d       2023-03-15      BREAKING CHANGE: Updated partition table: Make sure you have a configuration backup and completly reflash the device!
* 3b7aef6       2023-02-13      BREAKING CHANGE: Web API!
* d4c838a       2023-02-06      BREAKING CHANGE: Prometheus API!
* daf847e       2022-11-14      BREAKING CHANGE: Removed deprecated config parsing method
* 69b675b       2022-11-01      BREAKING CHANGE: Structure WebAPI /api/livedata/status changed
* 27ed4e3       2022-10-31      BREAKING: Change power factor from percent value to value between 0 and 1
```

## Currently supported Inverters

A list of all currently supported inverters can be found [here](https://www.opendtu.solar/hardware/inverter_overview/)
