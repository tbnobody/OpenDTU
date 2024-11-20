[![OpenDTU-OnBattery Build](https://github.com/hoylabs/OpenDTU-OnBattery/actions/workflows/build.yml/badge.svg)](https://github.com/hoylabs/OpenDTU-OnBattery/actions/workflows/build.yml)
[![cpplint](https://github.com/hoylabs/OpenDTU-OnBattery/actions/workflows/cpplint.yml/badge.svg)](https://github.com/hoylabs/OpenDTU-OnBattery/actions/workflows/cpplint.yml)
[![Yarn Linting](https://github.com/hoylabs/OpenDTU-OnBattery/actions/workflows/yarnlint.yml/badge.svg)](https://github.com/hoylabs/OpenDTU-OnBattery/actions/workflows/yarnlint.yml)
<!---
disabled while "create release badge" action is broken, see .github/build.yml
![GitHub tag (latest SemVer)](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/helgeerbe/68b47cc8c8994d04ab3a4fa9d8aee5e6/raw/openDTUcoreRelease.json)
--->

- [OpenDTU-OnBattery](#opendtu-onbattery)
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
inverter's power production to the actual houshold consumption. In this way, it
is possible to implement a zero export policy.

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
