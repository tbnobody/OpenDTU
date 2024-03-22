- [OpenDTU-onBattery](#opendtu-onbattery)
  - [What is OpenDTU-onBattery](#what-is-opendtu-onbattery)
  - [History of the project](#history-of-the-project)
  - [Highlights of OpenDTU-onBattery](#highlights-of-opendtu-onbattery)
  - [Documentation](#documentation)
  - [Acknowledgment](#acknowledgment)

# OpenDTU-onBattery

This is a fork from the Hoymiles project [OpenDTU](https://github.com/tbnobody/OpenDTU).

![GitHub tag (latest SemVer)](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/helgeerbe/68b47cc8c8994d04ab3a4fa9d8aee5e6/raw/openDTUcoreRelease.json)

[![OpenDTU-onBattery Build](https://github.com/helgeerbe/OpenDTU-OnBattery/actions/workflows/build.yml/badge.svg)](https://github.com/helgeerbe/OpenDTU-OnBattery/actions/workflows/build.yml)
[![cpplint](https://github.com/helgeerbe/OpenDTU-OnBattery/actions/workflows/cpplint.yml/badge.svg)](https://github.com/helgeerbe/OpenDTU-OnBattery/actions/workflows/cpplint.yml)
[![Yarn Linting](https://github.com/helgeerbe/OpenDTU-OnBattery/actions/workflows/yarnlint.yml/badge.svg)](https://github.com/helgeerbe/OpenDTU-OnBattery/actions/workflows/yarnlint.yml)

## What is OpenDTU-onBattery

OpenDTU-onBattery is an extension of the original OpenDTU to support battery chargers, battery management systems (BMS) and power meters on a single esp32. With the help of a dynamic power limiter, the power production can be adjusted to the actual consumption. In this way, it is possible to come as close as possible to the goal of zero feed-in.

## History of the project

The original OpenDTU project was started from [this](https://www.mikrocontroller.net/topic/525778) discussion (Mikrocontroller.net). It was the goal to replace the original Hoymiles DTU (Telemetry Gateway) with their cloud access. With a lot of reverse engineering the Hoymiles protocol was decrypted and analyzed.

Summer 2022 I bought my Victron MPPT battery charger, and didn't like the idea to set up a separate esp32 to recieve the charger data. I decided to fork OpenDTU and extend it with battery charger support and a dynamic power limitter to my own needs. Hoping someone can make use of it.

## Highlights of OpenDTU-onBattery

This project is still under development and adds following features:

* Support Victron's Ve.Direct protocol on the same chip (cable based serial interface!). Additional information about Ve.direct can be downloaded directly from [Victron's website](https://www.victronenergy.com/support-and-downloads/technical-information).
* Dynamically sets the Hoymiles power limited according to the currently used energy in the household. Needs an HTTP JSON based power meter (e.g. Tasmota), an MQTT based power meter like Shelly 3EM or an SDM power meter.
* Battery support: Read the voltage from Victron MPPT charge controller or from the Hoymiles DC inputs and starts/stops the power producing based on configurable voltage thresholds
* Voltage correction that takes the voltage drop because of the current output load into account (not 100% reliable calculation)
* Can read the current solar panel power from the Victron MPPT and adjust the limiter accordingly to not save energy in the battery (for increased system efficiency). Increases the battery lifespan and reduces energy loses.
* Settings can be configured in the UI
* Pylontech Battery support (via CAN bus interface). Use the SOC for starting/stopping the power output and provide the battery data via MQTT (autodiscovery for home assistant is currently not supported). Pin Mapping is supported (default RX PIN 27, TX PIN 26). Actual no live view support for Pylontech Battery.
* Huawei R4850G2 power supply unit that can act as AC charger. Supports status shown on the web interface and options to set voltage and current limits on the web interface and via MQTT. Connection is done using CAN bus (needs to be separate from Pylontech CAN bus) via SN65HVD230 interface.
  
## Documentation

[Full documentation of OpenDTU-onBattery extensions can be found at the project's wiki](https://github.com/helgeerbe/OpenDTU-OnBattery/wiki).

For documentation of openDTU core functionality I refer to the original [repo](https://github.com/tbnobody/OpenDTU) and its [documentation](https://tbnobody.github.io/OpenDTU-docs/).

Please note that openDTU-onBattery may change significantly during its development.
Bug reports, comments, feature requests and fixes are most welcome!

To find out what's new or improved have a look at the [changelog](https://github.com/helgeerbe/OpenDTU-OnBattery/releases).

## Acknowledgment

A special Thank to Thomas Basler (tbnobody) the author of the original [OpenDTU](https://github.com/tbnobody/OpenDTU) project. You are doing a great job!

Last but not least, I would like to thank all the contributors. With your ideas and enhancements, you have made OpenDTU-onBattery much more than I originally had in mind.
