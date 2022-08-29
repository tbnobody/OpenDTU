# OpenDTU_VeDirect

## Background
This project was started from [this](https://www.mikrocontroller.net/topic/525778) discussion (Mikrocontroller.net).
It was the goal to replace the original Hoymiles DTU (Telemetry Gateway) with their cloud access. With a lot of reverse engineering the Hoymiles protocol was decrypted and analyzed.

I extended the original OpenDTU software to support also Victron's Ve.Direct protocol on the same chip. Additional information about Ve.direct can be downloaded from https://www.victronenergy.com/support-and-downloads/technical-information.

Web-Live-Interface:
![image](https://user-images.githubusercontent.com/59169507/187224107-4e0d0cab-2e1b-4e47-9410-a49f80aa6789.png)

REST-API (/api/verdirectlivedata/status):
````JSON
{
"data_age":0,
"age_critical":false,
"PID":"SmartSolar MPPT 100|30",
"SER":"XXX",
"FW":"159",
"LOAD":"ON",
"CS":"Bulk",
"ERR":"No error",
"OR":"Not off",
"MPPT":"MPP Tracker active",
"HSDS":{"v":46,"u":"Days"},
"V":{"v":26.36,"u":"V"},
"I":{"v":3.4,"u":"A"},
"VPV":{"v":37.13,"u":"V"},
"PPV":{"v":93,"u":"W"},
"H19":{"v":83.16,"u":"kWh"},
"H20":{"v":1.39,"u":"kWh"},
"H21":{"v":719,"u":"W"},
"H22":{"v":1.43,"u":"kWh"},
"H23":{"v":737,"u":"W"}
}
````

MQTT:

Sends text raw data as difined in VE.Direct spec.

## Currently supported Inverters
* Hoymiles HM-300
* Hoymiles HM-350
* Hoymiles HM-400
* Hoymiles HM-600
* Hoymiles HM-700
* Hoymiles HM-800
* Hoymiles HM-1000
* Hoymiles HM-1200
* Hoymiles HM-1500
* TSUN TSOL-M800 (Maybe depending on firmware on the inverter)

## Features for end users
* Read live data from inverter
* Show inverters internal event log
* Show inverter information like firmware version, firmware build date, hardware revision and hardware version
* Uses ESP32 microcontroller and NRF24L01+
* Multi-Inverter support
* MQTT support (with TLS)
* Home Assistant MQTT Auto Discovery support
* Nice and fancy WebApp with visualization of current data
* Firmware upgrade using the web UI
* Default source supports up to 10 inverters
* Time zone support
* Ve.Direct interface (via web-interface, REST-api, or MQTT)


## Features for developers
* The microcontroller part
    * Build with Arduino PlatformIO Framework for the ESP32
    * Uses [ESPAsyncWebserver](https://github.com/me-no-dev/ESPAsyncWebServer) and [Async MQTT client](https://github.com/marvinroger/async-mqtt-client)

* The WebApp part
    * Build with [Vue.js](https://vuejs.org)
    * Source is written in TypeScript

## Wiring up
### Schematic
![Schematic](docs/Wiring_ESP32_Schematic.png)

### Symbolic view
![Symbolic](docs/Wiring_ESP32_Symbol.png)

### Change pin assignment
Its possible to change all the pins of the NRF24L01+ module.
This can be achieved by editing the 'platformio.ini' file and add/change one or more of the following lines to the 'build_flags' parameter:
```
-DHOYMILES_PIN_MISO=19
-DHOYMILES_PIN_MOSI=23
-DHOYMILES_PIN_SCLK=18
-DHOYMILES_PIN_IRQ=16
-DHOYMILES_PIN_CE=4
-DHOYMILES_PIN_CS=5
-DVICTRON_PIN_TX=21
-DVICTRON_PIN_RX=22 
```

## Flashing and starting up
### with Visual Studio Code
* Install [Visual Studio Code](https://code.visualstudio.com/download)
* In Visual Studio Code, install the [PlatformIO Extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide)
* Clone this repository (you really have to clone it, don't just download the ZIP file. During the build process the git hash gets embedded into the firmware. If you download the ZIP file a build error will occur)
* In Visual Studio Code, choose File --> Open Folder and select the previously downloaded source code. (You have to select the folder which contains the "platformio.ini" file)
* Adjust the COM port in the file "platformio.ini". It occurs twice:
    * upload_port
    * monitor_port
* Select the arrow button in the status bar (PlatformIO: Upload) to compile and upload the firmware. During the compilation, all required libraries are downloaded automatically.
### on the commandline with PlatformIO Core
* Install [PlatformIO Core](https://platformio.org/install/cli)
* Clone this repository (you really have to clone it, don't just download the ZIP file. During the build process the git hash gets embedded into the firmware. If you download the ZIP file a build error will occur)
* Adjust the COM port in the file "platformio.ini". It occurs twice:
    * upload_port
    * monitor_port
* build: `platformio run -e generic`
* upload to esp module: `platformio run -e generic -t upload`
* other options:
  * clean the sources:  `platformio run -e generic -t clean`
  * erase flash: `platformio run -e generic -t erase`

## First configuration
* After the initial flashing of the microcontroller, an Access Point called "OpenDTU-*" is opened. The default password is "openDTU42".
* Use a web browser to open the address [http://192.168.4.1](http://192.168.4.1)
* Navigate to Settings --> Network Settings and enter your WiFi credentials
* Currently you have to look at your router to determine the IP of the newly connected device

## Available cases
* [https://www.thingiverse.com/thing:5435911](https://www.thingiverse.com/thing:5435911)

## Building
* Building the WebApp
    * The WebApp can be build using yarn
    ```
    $ cd webapp
    $ yarn install
    $ yarn build
    ```
    * The updated output is placed in the 'webapp_dist' directory
    * It is only necessary to build the webapp when you made changes to it

* Building the microcontroller firmware
    * Visual Studio Code with the PlatformIO Extension is required for building
