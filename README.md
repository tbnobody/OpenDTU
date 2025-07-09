[![OpenDTU-OnBattery Build](https://github.com/hoylabs/OpenDTU-OnBattery/actions/workflows/build.yml/badge.svg)](https://github.com/hoylabs/OpenDTU-OnBattery/actions/workflows/build.yml)
[![cpplint](https://github.com/hoylabs/OpenDTU-OnBattery/actions/workflows/cpplint.yml/badge.svg)](https://github.com/hoylabs/OpenDTU-OnBattery/actions/workflows/cpplint.yml)
[![Yarn Linting](https://github.com/hoylabs/OpenDTU-OnBattery/actions/workflows/yarnlint.yml/badge.svg)](https://github.com/hoylabs/OpenDTU-OnBattery/actions/workflows/yarnlint.yml)
![GitHub tag (latest SemVer)](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/AndreasBoehm/856dda48c1cadac6ea495213340c612b/raw/openDTUcoreRelease.json)

- [OpenDTU-OnBattery](#opendtu-onbattery)
  - [About Hardware For Sale](#%EF%B8%8F--about-hardware-for-sale)
  - [Getting Started](#getting-started)
  - [Important Differences](#important-differences)
  - [Documentation](#documentation)
  - [Project State](#project-state)
  - [Project History](#project-history)
  - [Acknowledgments](#acknowledgments)

# OpenDTU-OnBattery

OpenDTU-OnBattery is a fork of [OpenDTU](https://github.com/tbnobody/OpenDTU),
which adds support for battery chargers, battery management systems (BMS), and
power meters on a single ESP32. Its Dynamic Power Limiter can adjust the
power production of one or more solar- and/or battery-powered inverter(s) to
the actual household consumption. In this way, it is possible to implement a
zero export policy.

## Inverter Firmware Compatibility

⚠️ **WARNING: Avoid firmware version 2.0.4** ⚠️

| Version | PDL\*) | Temporary Limit | Persistent Limit | Recommendation                  |
|:--------|:------:|:---------------:|:----------------:|---------------------------------|
| 1.0.x   | ❌     | ✅              | ✅               | Best Option if PDL not required |
| 1.1.12  | ✅     | ✅              | ❌               | Not recommended                 |
| 2.0.4   | ❌     | ❌              | ❌               | Avoid/Downgrade                 |

\*) PDL = Power Distribution Logic, i.e., the inverter's ability to limit the
inputs individually to achieve the desired AC output power.

**Key Issues:**
- **Version 1.1.12**: PDL works fine, but persistent limit changes are not
  supported ([#1890](https://github.com/hoylabs/OpenDTU-OnBattery/issues/1890)).
- **Version 2.0.4**: Inverter reports 100% power limit after 4 minutes without
  limit updates, causing the DPL to stop working
  ([#1901](https://github.com/hoylabs/OpenDTU-OnBattery/issues/1901)).

**Recommendation**: Version 1.1.12 has working PDL but is still not recommended
due to persistent limit issues. Avoid version 2.0.4 completely.

## ⚠️  About Hardware For Sale

### Do It Yourself

You are always welcome to select and use your own set of parts to run
OpenDTU-OnBattery. Our [documentation](https://opendtu-onbattery.net/hardware/)
should give you the information you need. There are also [case
designs](https://opendtu-onbattery.net/3rd_party/cases/) available for you to
print yourself.

### Ready-To-Use

If you are interested in ready-to-use hardware available for sale, the
OpenDTU-OnBattery project endorses the **[OpenDTU Fusion
board](https://opendtu-onbattery.net/3rd_party/opendtu_fusion/)**.

### Distribution Channels

OpenDTU Fusion boards and accessories (cases in particular) are available
exclusively through:

- [AllianceApps](https://shop.allianceapps.io/)
- [Semy3D](https://shop.semy3d.de/)

Please note that we do not have any connections to hardware sold on platforms
like eBay, Kleinanzeigen, or similar marketplaces. Items sold there may not
meet the quality and compatibility standards expected by our community.

### Why Purchase Through the Endorsed Channels?

Buying from the trusted sources listed above ensures:

1. Access to reliable, well-tested hardware that meets the project’s requirements.
2. Support for suppliers who align with the values of the OpenDTU community.

To allow for a good overall experience, we encourage users to avoid unverified
hardware sources.

### 🚨 Misleading Claims

We are aware that some sellers falsely advertise that purchasing their hardware
supports the OpenDTU-OnBattery, OpenDTU, and/or AhoyDTU developers financially.
**This is not true.** The OpenDTU-OnBattery project does **not** receive any
financial benefit from the sale of hardware.

## Getting Started

See the documentation to learn [what hardware](https://opendtu-onbattery.net/hardware/)
to acquire, how to [initialize](https://opendtu-onbattery.net/firmware/) it
with OpenDTU-OnBattery firmware, and how to
[configure](https://opendtu-onbattery.net/firmware/device_profiles/)
OpenDTU-OnBattery for your hardware.

## Important Differences

Generally speaking, OpenDTU-OnBattery and the upstream project are compatible
with each other, because OpenDTU-OnBattery mostly only extends the upstream
project. However, there are a few notable differences aside from the added functionality:

* OpenDTU-OnBattery, due to its code footprint, cannot offer support for
  over-the-air (OTA) updates on ESP32 with only 4MB of flash memory. Consult
  the [documentation](https://opendtu-onbattery.net/firmware/howto/upgrade_8mb/#background)
  to learn more.
* Unlike in the upstream project, you **must** compile the web application
  yourself when attempting to build your own firmware blob. See the
  [documentation](https://opendtu-onbattery.net/firmware/compile_webapp/) for
  details.

## Documentation

The canonical documentation of OpenDTU-OnBattery is hosted at
[https://opendtu-onbattery.net](https://opendtu-onbattery.net).

You may find additional helpful information in the project's
community-maintained [Github
Wiki](https://github.com/hoylabs/OpenDTU-OnBattery/wiki).

To find out what's new or improved have a look at the
[releases](https://github.com/hoylabs/OpenDTU-OnBattery/releases).

## Project State

OpenDTU-OnBattery is actively maintained. Please note that OpenDTU-OnBattery
may change significantly during its development. Bug reports, comments, feature
requests and pull requests are welcome!

## Project History

The original OpenDTU project was started from [a discussion on
Mikrocontroller.net](https://www.mikrocontroller.net/topic/525778). The
original ambition was to replace the original Hoymiles DTU (Telemetry Gateway)
to avoid using Hoymile's cloud. With a lot of reverse engineering, the Hoymiles
protocol was decrypted and analyzed.

In the summer of 2022 [@helgeerbe](https://github.com/helgeerbe) bought a
Victron MPPT charge controller, and didn't like the idea to set up a separate
ESP32 to receive the charger's data. He decided to fork OpenDTU and extend it
with battery charger support and a Dynamic Power Limiter.

In early October 2024, the project moved to the newly founded GitHub
organisation `hoylabs` and is since maintained by multiple community members.

## Acknowledgments

* Special thanks to Thomas Basler ([@tbnobody](https://github.com/tbnobody)),
  the author of the [upstream project](https://github.com/tbnobody/OpenDTU),
  for his continued effort!
* Thanks to [@helgeerbe](https://github.com/helgeerbe) for starting
  OpenDTU-OnBattery, for his dedication to the project, as well as for his
  trust in the current maintainers of the project, which act as part of the
  `hoylabs` GitHub organisation.
* We like to thank all contributors. With your ideas and enhancements, you have
  made OpenDTU-OnBattery much more than
  [@helgeerbe](https://github.com/helgeerbe) originally had in mind.
