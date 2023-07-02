# OpenDTU-OnBattery

This is a fork from the Hoymiles project [OpenDTU](https://github.com/tbnobody/OpenDTU). This project is still under development but is being used on a day to day basis as well.

![GitHub tag (latest SemVer)](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/helgeerbe/68b47cc8c8994d04ab3a4fa9d8aee5e6/raw/openDTUcoreRelease.json)

> **Warning**
> 
> In contrast to the original openDTU, with release 2023.05.23.post1 openDTU-onBattery supports only 5 inverters. Otherwise, there is not enough memory for the liveData view.

## Features

* Hoymiles inverter support for live data and display of various inverter internal information. (Partial) support for multiple inverters.
* MQTT support (with TLS) with partial Home Assistant MQTT Auto Discovery
* Automatic inverter power control of a selected Hoymiles inverter to compensate the currently used energy in the household. 
* Energy meter support with interface options to HTTP JSON based power meters (e.g. Tasmota), MQTT based power meters (e.g. Shelly 3EM) or SDM power meters.
* Support for Victron MPPT charge controller using Ve.Direct. cf. Ve.direct: https://www.victronenergy.com/support-and-downloads/technical-information.
* Generic voltage based battery support using Victron MPPT charge controller or Hoymiles inverter voltage values to start / stop inverter power output. (with load compensation)
* Pylontech battery support via CAN bus interface. State of Charge reported by BMS is used to start / stop inverter power output. Battery data is exported via MQTT (no support for home assistant auto discovery). 
* Support for Huawei R4850G2 power supply unit that can act as AC charging source. [Overview](https://www.beyondlogic.org/review-huawei-r4850g2-power-supply-53-5vdc-3kw/)
* Other features from [OpenDTU](https://github.com/tbnobody/OpenDTU) maintained

## Hardware

To get started with this project you will need to assemble a few hardware components that allow interfacing with the desired devices. What is needed depends on the use-case but may consist of:

* ESP32 board that contains the CPU and WIFI connectivity
* NRF24L01+ or CMT2300A radio board to interface with the inverter. Please check the list of the supported inverters below for the board needed.
* 3.3V / 5V logic level shifter to interface with the Victron MPPT charge controller
* SN65HVD230 CAN bus transceiver to interface with a Pylontech battery
* MCP2515 SPI / CAN bus transceiver to interface with the Huawei AC PSU
* Relais board + 3.3V / 5 V logic level shifter to switch the slot detect on the Huawei AC PSU
* Display [Display](docs/Display.md)

More detailed information on the hardware can be found in the [Hardware and flashing](docs/hardware_flash.md) document.

### Currently supported Inverters

| Model               | Required RF Module | DC Inputs | MPP-Tracker | AC Phases |
| --------------------| ------------------ | --------- | ----------- | --------- |
| Hoymiles HM-300     | NRF24L01+          | 1         | 1           | 1         |
| Hoymiles HM-350     | NRF24L01+          | 1         | 1           | 1         |
| Hoymiles HM-400     | NRF24L01+          | 1         | 1           | 1         |
| Hoymiles HM-600     | NRF24L01+          | 2         | 2           | 1         |
| Hoymiles HM-700     | NRF24L01+          | 2         | 2           | 1         |
| Hoymiles HM-800     | NRF24L01+          | 2         | 2           | 1         |
| Hoymiles HM-1000    | NRF24L01+          | 4         | 2           | 1         |
| Hoymiles HM-1200    | NRF24L01+          | 4         | 2           | 1         |
| Hoymiles HM-1500    | NRF24L01+          | 4         | 2           | 1         |
| Hoymiles HMS-300    | CMT2300A           | 1         | 1           | 1         |
| Hoymiles HMS-350    | CMT2300A           | 1         | 1           | 1         |
| Hoymiles HMS-400    | CMT2300A           | 1         | 1           | 1         |
| Hoymiles HMS-450    | CMT2300A           | 1         | 1           | 1         |
| Hoymiles HMS-500    | CMT2300A           | 1         | 1           | 1         |
| Hoymiles HMS-600    | CMT2300A           | 2         | 2           | 1         |
| Hoymiles HMS-700    | CMT2300A           | 2         | 2           | 1         |
| Hoymiles HMS-800    | CMT2300A           | 2         | 2           | 1         |
| Hoymiles HMS-900    | CMT2300A           | 2         | 2           | 1         |
| Hoymiles HMS-1000   | CMT2300A           | 2         | 2           | 1         |
| Hoymiles HMS-1600   | CMT2300A           | 4         | 4           | 1         |
| Hoymiles HMS-1800   | CMT2300A           | 4         | 4           | 1         |
| Hoymiles HMS-2000   | CMT2300A           | 4         | 4           | 1         |
| Hoymiles HMT-1800   | CMT2300A           | 6         | 3           | 3         |
| Hoymiles HMT-2250   | CMT2300A           | 6         | 3           | 3         |
| Solenso SOL-H350    | NRF24L01+          | 1         | 1           | 1         |
| Solenso SOL-H400    | NRF24L01+          | 1         | 1           | 1         |
| Solenso SOL-H800    | NRF24L01+          | 2         | 2           | 1         |
| TSUN TSOL-M350      | NRF24L01+          | 1         | 1           | 1         |
| TSUN TSOL-M800      | NRF24L01+          | 2         | 2           | 1         |
| TSUN TSOL-M1600     | NRF24L01+          | 4         | 2           | 1         |

**TSUN compatibility remark:**
Compatibility with OpenDTU is most likely related to the serial number of the inverter. Current findings indicate that TSUN inverters with a serial number starting with "11" are supported, whereby inverters with a serial number starting with "10" are not.


## Screenshots

Several screenshots of the frontend can be found here: [Screenshots](docs/screenshots/README.md)

## Configuration and usage

### First configuration

* After the [initial flashing](docs/hardware_flash.md#flashing-and-starting-up) of the microcontroller, an Access Point called "OpenDTU-*" is opened. The default password is "openDTU42".
* Use a web browser to open the address [http://192.168.4.1](http://192.168.4.1)
* Navigate to Settings --> Network Settings and enter your WiFi credentials. The username to access the config menu is "admin" and the password the same as for accessing the Access Point (default: "openDTU42").
* OpenDTU then simultaneously connects to your WiFi AP with these credentials. Navigate to Info --> Network and look into section "Network Interface (Station)" for the IP address received via DHCP.
* If your WiFi AP uses an allow-list for MAC-addresses, please be aware that the ESP32 has two different MAC addresses for its AP and client modes, they are also listed at Info --> Network.
* When OpenDTU is connected to a configured WiFI AP, the "OpenDTU-*" Access Point is closed after 3 minutes.
* OpenDTU needs access to a working NTP server to get the current date & time. Both are sent to the inverter with each request. Default NTP server is pool.ntp.org. If your network has different requirements please change accordingly (Settings --> NTP Settings).
* Activate Ve.direct, Battery and the AC Charger according to the available hardware 
* Configure a Power Meter to provide a data source for the current consumption
* Configure the Dynamic Power Limiter according to the used battery. Documentation about the power limiter interface and states can be found below.
* If desired connect to a home automation system using MQTT or the Webapi. 
  * A documentation of all available MQTT Topics can be found here: [MQTT Documentation](docs/MQTT_Topics.md)
  * A documentation of the Web API can be found here: [Web-API Documentation](docs/Web-API.md)
  * Home Assistant auto discovery is supported. [Example image](https://user-images.githubusercontent.com/59169507/217558862-a83846c5-6070-43cd-9a0b-90a8b2e2e8c6.png)

### Dynamic Power Limiter

The dynamic power limiter is responsible for automatic inverter power adjustment. It will take the Power Meter (i.e. currently consumed power), the solar power and the battery charge state into account. The dynamic power limiter supports a few different strategies that can be configured from the user interface:

* Solar Passthrough is off
  * When using this strategy the inverter is steered such that the currently consumed power (as provided by the power meter) is compensated for. This is done as long as the battery charge state is above the limit set by the stop threshold. The inverter is turned off if the battery reaches the limit and is only re-enabled if the battery charge state reaches the limit set by start threshold.  
* Solar Passthrough is on and the battery drain strategy is empty when full
  * This case applies the same strategy as the strategy above. In addition Solar Power will be used to compensate for the currently used energy in cases where the battery discharge is disabled. In this case the inverter power limit is constrained to the input solar power and the power meter value so that battery discharge is avoided.
* Solar Passthrough is on and the battery drain strategy is empty at night
  * When using this strategy the inverter is steered such that the currently consumed power (as provided by the power meter) is compensated for. During daytime energy is taken from solar and from the battery, if the battery level is above the start threshold. At night battery power is used until the battery level reaches the stop threshold. When operating on solar power only (i.e. without using the battery) the inverter power limit is constrained to the input solar power and the power meter value so that battery discharge is avoided. The daytime / nighttime switch is based on the Victron MPPT Solar Charger power and 20W input are required in this case. 

Other settings are:
* The inverter ID configures the inverter that is controlled by the power limiter. The power limiter can only control a single inverter at this point in time.
* Channel ID is the inverter input channel ID that is used for battery voltage readings.
* Target power consumption specifies the power to be either consumed from the grid (when set to a positive value) or fed back into the grid (when set to a negative value).
* The hysteresis value helps optimize communication with the inverter by skipping unnecessary power limit updates. An update is only sent if the absolute difference between the newly computed power limit and the previously set limit matches or exceeds the hysteresis value. This approach can conserve both airtime and CPU resources.
* Power limits control the min / max limits of the inverter
* Inverter is behind power meter. Select this if your inverter power is measured by the power meter. This is typically the case.
* Battery start and stop threshold can be configured using voltage and / or state of charge values. Stage of charge values requires a Pylontech battery at this point.
* A Battery full solar passthrough threshold can be configured using voltage or state of charge value. Stage of charge values requires a Pylontech battery at this point. The option can be used if the battery is full and will steer the inverter according to solar power reported by the Victron MPPT Solar Charger. 

![image](https://user-images.githubusercontent.com/59169507/222155765-9fff47a4-8ffa-42cf-8671-6359288e0cab.png)

#### Power Limiter States
![PowerLimiterInverterStates](https://github.com/helgeerbe/OpenDTU-OnBattery/blob/development/docs/PowerLimiterInverterStates.png)

### Huawei PSU 

The Huawei PSU can be used to charge a battery. This can be be useful if an external (AC) connected solar system shall be utilized or if variable energy prices should be exploited. 

Some points for consideration are: 
* Make sure to consider the PSU voltage range when selecting the battery voltage as lower voltages <42V are not supported. 
* The PSU runs a noisy fan and it is therefore desireable to switch it off when not being used. Some users have found that switching the slot detect pins with a relay accomplishes this. A GPIO pin is made available from the ESP to turn the PSU on/off

#### Operation modes

openDTU-onBattery supports three operation modes for the Huawei PSU:
1. Fully manual - In this mode the PSU needs to be turned on/off externally using MQTT and voltage and current limits need to be provided. See [MQTT Documentation](docs/MQTT_Topics.md) for details on these commands
2. Manual with auto power on / off - In this mode the PSU is turned on when a current limit > 1A is set. If the current limit is < 1A for some time the PSU is turned off. Current and voltage limits need to be provided externally using MQTT. See [MQTT Documentation](docs/MQTT_Topics.md) for details on these commands.
3. Automatic - In this mode the PSU power is controlled by the Power Meter and information provided in the web-interface. If excess power is present the PSU is turned on. The voltage limit is set as per web-interface and the current limit is set so that the maximum PSU output power equals the Power Meter value. Minium and maximum PSU power levels as configured in the web-interface are respected in this process. The PSU is turned off if the output current is limited and the output power drops below the minium power level. This will disable automatic mode until the battery is discharged below the start voltage level (set in the web-interface). This mode can be enabled using the web-interface and MQTT. See [MQTT Documentation](docs/MQTT_Topics.md)

## Troubleshooting

* First: When there is no light on the solar panels, the inverter completely turns off and does not answer to OpenDTU! So if you assembled your OpenDTU in the evening, wait until tomorrow.
* When there is no data received from the inverter(s) - try to reduce the distance between the openDTU and the inverter (e.g. move it to the window towards the roof)
* Under Settings -> DTU Settings you can increase the transmit power "PA level". Default is "minimum".
* The NRF24L01+ needs relatively much current. With bad power supply (and especially bad cables!) a 10 µF capacitor soldered directly to the NRF24L01+ board connector brings more stability (pin 1+2 are the power supply). Note the polarity of the capacitor…
* You can try to use an USB power supply with 1 A or more instead of connecting the ESP32 to the computer.
* Try a different USB cable. Once again, a stable power source is important. Some USB cables are made of much plastic and very little copper inside.
* Double check that you have a radio module NRF24L01+ with a plus sign at the end. NRF24L01 module without the plus are not compatible with this project.
* There is no possibility of auto-discovering the inverters. Double check you have entered the serial numbers of the inverters correctly.
* OpenDTU needs access to a working NTP server to get the current date & time.
* If your problem persists, check the  [Issues on Github](https://github.com/tbnobody/OpenDTU/issues). Please inspect not only the open issues, also the closed issues contain useful information.
* Another source of information are the [Discussions](https://github.com/tbnobody/OpenDTU/discussions/)
* When flashing with VSCode Plattform.IO fails and also with ESPRESSIF tool a demo bin file cannot be flashed to the ESP32 with error message "A fatal error occurred: MD5 of file does not match data in flash!" than un-wire/unconnect ESP32 from the NRF24L01+ board. Try to flash again and rewire afterwards.

## Background

This project was started from [this](https://www.mikrocontroller.net/topic/525778) discussion (Mikrocontroller.net).
It was the goal to replace the original Hoymiles DTU (Telemetry Gateway) with their cloud access. With a lot of reverse engineering the Hoymiles protocol was decrypted and analyzed.

## Features for developers

### Status

[![OpenDTU-onBattery Build](https://github.com/helgeerbe/OpenDTU-OnBattery/actions/workflows/build.yml/badge.svg)](https://github.com/helgeerbe/OpenDTU-OnBattery/actions/workflows/build.yml)
[![cpplint](https://github.com/helgeerbe/OpenDTU-OnBattery/actions/workflows/cpplint.yml/badge.svg)](https://github.com/helgeerbe/OpenDTU-OnBattery/actions/workflows/cpplint.yml)
[![Yarn Linting](https://github.com/helgeerbe/OpenDTU-OnBattery/actions/workflows/yarnlint.yml/badge.svg)](https://github.com/helgeerbe/OpenDTU-OnBattery/actions/workflows/yarnlint.yml)

### Core technologies used

* The microcontroller part
  * Build with Arduino PlatformIO Framework for the ESP32
  * Uses a fork of [ESPAsyncWebserver](https://github.com/yubox-node-org/ESPAsyncWebServer) and [espMqttClient](https://github.com/bertmelis/espMqttClient)

* The WebApp part
  * Build with [Vue.js](https://vuejs.org)
  * Source is written in TypeScript

### Breaking changes

Generated using: `git log --date=short --pretty=format:"* %h%x09%ad%x09%s" | grep BREAKING`

```code
* 59f43a8       2023-04-17      BREAKING CHANGE: Web API Endpoint /api/devinfo/status requires GET parameter inv=
* 318136d       2023-03-15      BREAKING CHANGE: Updated partition table: Make sure you have a configuration backup and completly reflash the device!
* 3b7aef6       2023-02-13      BREAKING CHANGE: Web API!
* d4c838a       2023-02-06      BREAKING CHANGE: Prometheus API!
* daf847e       2022-11-14      BREAKING CHANGE: Removed deprecated config parsing method
* 69b675b       2022-11-01      BREAKING CHANGE: Structure WebAPI /api/livedata/status changed
* 27ed4e3       2022-10-31      BREAKING: Change power factor from percent value to value between 0 and 1
```

### Building

* Building the WebApp
  * The WebApp can be build using yarn

    ```bash
    cd webapp
    yarn install
    yarn build
    ```

  * The updated output is placed in the 'webapp_dist' directory
  * It is only necessary to build the webapp when you made changes to it
* Building the microcontroller firmware
  * Visual Studio Code with the PlatformIO Extension is required for building

## Related Projects

* [Ahoy](https://github.com/grindylow/ahoy)
* [DTU Simulator](https://github.com/Ziyatoe/DTUsimMI1x00-Hoymiles)
* [OpenDTU extended to talk to Victrons MPPT battery chargers (Ve.Direct)](https://github.com/helgeerbe/OpenDTU_VeDirect)
