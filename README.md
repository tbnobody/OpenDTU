# OpenDTU

## Background
This project was started from [this](https://www.mikrocontroller.net/topic/525778) discussion (Mikrocontroller.net).
It was the goal to replace the original Hoymiles DTU (Telemetry Gateway) with their cloud access. With a lot of reverse engineering the Hoymiles protocol was decrypted and analyzed.

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
* OpenDTU then simultaneously connects to your WiFi AP with this credentials. Navigate to Info --> Network and look into section "Network Interface (Station)" for the IP address received via DHCP.
* When OpenDTU is connected to a configured WiFI AP, the "OpenDTU-*" Access Point is closed after 3 minutes.
* OpenDTU needs access to a working NTP server to get the current date & time. Both are sent to the inverter with each request. Default NTP server is pool.ntp.org. If your network has different requirements please change accordingly (Settings --> NTP Settings).
* Add your inverter in the inverter settings (Settings --> Inverter Settings)

## Flashing an Update using "Over The Air" OTA Update
Once you have your OpenDTU running and connected to WLAN, you can do further updates through the web interface.
Navigate to Settings --> Firmware upgrade and press the browse button. Select the firmware file from your local computer.

You'll find the firmware file (after a successfull build process) under `.pio/build/generic/firmware.bin`.

After the successful upload, the OpenDTU immediately restarts into the new firmware.


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
