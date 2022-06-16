# OpenDTU

## Background
This project was started from [this](https://www.mikrocontroller.net/topic/525778) discussion (Mikrocontroller.net).
It was the goal to replace the original Hoymiles DTU (Telemetry Gateway) with their cloud access. With a lot of reverse engineering the Hoymiles protocol was decrypted and analyzed.


## Features for end users
* Uses ESP32 mikrocontroller and NRF24L01+
* Multi-Inverter support
* MQTT support
* Nice and fancy WebApp with visualisation of current data
* Firmware upgrade using the web UI
* Default source supports up to 10 inverters
* Timezone support


## Features for developers
* The mikrocontroller part
    * Build with Arduino PlatformIO Framework for the ESP32
    * Uses [ESPAsyncWebserver](https://github.com/me-no-dev/ESPAsyncWebServer) and [Async MQTT client](https://github.com/marvinroger/async-mqtt-client)

* The WebApp part
    * Build with [Vue.js](https://vuejs.org)

## Flashing and starting up

## First configuration
* After the initial flashing of the mikrocontroller, a Access Point called "OpenDTU-*" is opened. The default password is "openDTU42".
* Use a webbrowser to open the address [http://192.168.4.1](http://192.168.4.1)
* Navigate to Settings --> Network Settings and enter your WiFi credentials
* Currently you have to look at your router to determine the IP of the newly connected device

## Building
* Building the WebApp
    * The WebApp can be build using yarn
    ```
    $ yarn install
    $ yarn build
    ```
    * The updated output is placed in the 'data' directory

* Building the mikrocontroller firmware
    * Visual Studio Code with the PlatformIO Extension is required for building
