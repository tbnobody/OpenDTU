# Hardware, building and flashing Firmware

## Hardware you need

### ESP32 board

For ease of use, buy a "ESP32 DEVKIT DOIT" or "ESP32 NodeMCU Development Board" with an ESP32-S3 or ESP-WROOM-32 chipset on it.

Sample Picture:

![NodeMCU-ESP32](nodemcu-esp32.png)

Also supported: Board with Ethernet-Connector and Power-over-Ethernet [Olimex ESP32-POE](https://www.olimex.com/Products/IoT/ESP32/ESP32-POE/open-source-hardware)

### NRF24L01+ radio board (See inverter table above for supported inverters)

The PLUS sign is IMPORTANT! There are different variants available, with antenna on the printed circuit board or external antenna.

Sample picture:

![nrf24l01plus](nrf24l01plus.png)

Buy your hardware from a trusted source, at best from a dealer/online shop in your country where you have support and the right to return non-functional hardware.
When you want to buy from Amazon, AliExpress, eBay etc., take note that there is a lot of low-quality or fake hardware offered. Read customer comments and ratings carefully!

A heavily incomplete list of trusted hardware shops in Germany is:

* [AZ-Delivery](https://www.az-delivery.de/)
* [Makershop](https://www.makershop.de/)
* [Berrybase](https://www.berrybase.de/)

This list is for your convenience only, the project is not related to any of these shops.

### CMT2300A radio board  (See inverter table above for supported inverters)

It is important to get a module which supports SPI communication. The following modules are currently supported:

* EBYTE E49-900M20S

The CMT2300A uses 3-Wire half duplex SPI communication. Due to this fact it currently requires a separate SPI bus. If you want to run the CMT2300A module on the same ESP32 as a NRF24L01+ module or a PCD8544 display make sure you get a ESP which supports 2 SPI busses. Currently the SPI bus host is hardcoded to number 2. This may change in future.

### 3.3V / 5V logic level converter

The logic level converter is used to interface with the Victron MPPT charge controller and the relay board. It converts the 3.3V logic level used by the ESP32 to 5V logic used by the other devices. 

### SN65HVD230 CAN bus transceiver

The SN65HVD230 CAN bus transceiver is used to interface with the Pylontech battery. It leverages the CAN bus controller of the ESP32. This CAN bus operates at 500kbit/s

### MCP2515 CAN bus module

See [Wiki](https://github.com/helgeerbe/OpenDTU-OnBattery/wiki/Huawei-AC-PSU) for details.

### Relay module

The Huawei PSU can be switched on / off using the slot detect port. This is done by this relay.

### Power supply

Use a power supply with 5 V and 1 A. The USB cable connected to your PC/Notebook may be powerful enough or may be not.

## Wiring up the NRF24L01+ module

### Schematic

![Schematic](Wiring_ESP32_Schematic.png)

### Symbolic view

![Symbolic](Wiring_ESP32_Symbol.png)

### Change pin assignment

Its possible to change all the pins of the NRF24L01+ module, the Display, the LED etc.
The recommend way to change the pin assignment is by creating a custom [device profile](DeviceProfiles.md).
It is also possible to create a custom environment and compile the source yourself. This can be achieved by copying one of the [env:....] sections from 'platformio.ini' to 'platformio_override.ini' and editing the 'platformio_override.ini' file and add/change one or more of the following lines to the 'build_flags' parameter:

```makefile
-DHOYMILES_PIN_MISO=19
-DHOYMILES_PIN_MOSI=23
-DHOYMILES_PIN_SCLK=18
-DHOYMILES_PIN_IRQ=16
-DHOYMILES_PIN_CE=4
-DHOYMILES_PIN_CS=5
-DVICTRON_PIN_TX=21
-DVICTRON_PIN_RX=22
-DPYLONTECH_PIN_RX=27
-DPYLONTECH_PIN_TX=14
-DHUAWEI_PIN_MISO=12
-DHUAWEI_PIN_MOSI=13
-DHUAWEI_PIN_SCLK=26
-DHUAWEI_PIN_IRQ=25
-DHUAWEI_PIN_CS=15
-DHUAWEI_PIN_POWER=33
```

It is recommended to make all changes only in the  'platformio_override.ini', this is your personal copy.

## Flashing and starting up

### with Visual Studio Code

* Install [Visual Studio Code](https://code.visualstudio.com/download) (from now named "vscode")
* In Visual Studio Code, install the [PlatformIO Extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide)
* Install git and enable git in vscode - [git download](https://git-scm.com/downloads/) - [Instructions](https://www.jcchouinard.com/install-git-in-vscode/)
* Clone this repository (you really have to clone it, don't just download the ZIP file. During the build process the git hash gets embedded into the firmware. If you download the ZIP file a build error will occur): Inside vscode open the command palette by pressing `CTRL` + `SHIFT` + `P`. Enter `git clone`, add the repository-URL `https://github.com/tbnobody/OpenDTU`. Next you have to choose (or create) a target directory.
* In vscode, choose File --> Open Folder and select the previously downloaded source code. (You have to select the folder which contains the "platformio.ini" and "platformio_override.ini" file)
* Adjust the COM port in the file "platformio_override.ini" for your USB-to-serial-converter. It occurs twice:
  * upload_port
  * monitor_port
* Select the arrow button in the blue bottom status bar (PlatformIO: Upload) to compile and upload the firmware. During the compilation, all required libraries are downloaded automatically.
* Under Linux, if the upload fails with error messages "Could not open /dev/ttyUSB0, the port doesn't exist", you can check via ```ls -la /dev/tty*``` to which group your port belongs to, and then add your user this group via ```sudo adduser <yourusername> dialout``` (if you are using ```arch-linux``` use: ```sudo gpasswd -a <yourusername> uucp```, this method requires a logout/login of the affected user).
* There are two videos showing these steps:
  * [Git Clone and compilation](https://youtu.be/9cA_esv3zeA)
  * [Full installation and compilation](https://youtu.be/xs6TqHn7QWM)

### on the commandline with PlatformIO Core

* Install [PlatformIO Core](https://platformio.org/install/cli)
* Clone this repository (you really have to clone it, don't just download the ZIP file. During the build process the git hash gets embedded into the firmware. If you download the ZIP file a build error will occur)
* Adjust the COM port in the file "platformio_override.ini". It occurs twice:
  * upload_port
  * monitor_port
* build: `platformio run -e generic`
* upload to esp module: `platformio run -e generic -t upload`
* other options:
  * clean the sources:  `platformio run -e generic -t clean`
  * erase flash: `platformio run -e generic -t erase`

### using the pre-compiled .bin files

The pre-compiled files can be found on the [github page](https://github.com/tbnobody/OpenDTU) in the tab "Actions" and the sub menu "OpenDTU Build". Just choose the latest build from the master branch (search for "master" in the blue font text but click on the white header text!). You need to be logged in with your github account to download the files.
Use a ESP32 flash tool of your choice (see next chapter) and flash the `.bin` files to the right addresses:

| Address  | File                   |
| ---------| ---------------------- |
| 0x1000   | bootloader.bin         |
| 0x8000   | partitions.bin         |
| 0xe000   | boot_app0.bin          |
| 0x10000  | opendtu-*.bin          |

For further updates you can just use the web interface and upload the `opendtu-*.bin` file.

#### Flash with esptool.py (Linux)

```bash
esptool.py --port /dev/ttyUSB0 --chip esp32 --before default_reset --after hard_reset \
  write_flash --flash_mode dout --flash_freq 40m --flash_size detect \
  0x1000 bootloader.bin \
  0x8000 partitions.bin \
  0xe000 boot_app0.bin \
  0x10000 opendtu-generic.bin
```

#### Flash with Espressif Flash Download Tool (Windows)

[Download link](https://www.espressif.com/en/support/download/other-tools)

* On startup, select Chip Type -> "ESP32" / WorkMode -> "Develop"
* Prepare all settings (see picture). Make sure to uncheck the `DoNotChgBin` option. Otherwise you may get errors like "invalid header".
* ![flash tool image](esp32_flash_download_tool.png)
* Press "Erase" button on screen. Look into the terminal window, you should see dots appear. Then press  the "Boot" button on the ESP32 board. Wait for "FINISH" to see if flashing/erasing is done.
* To program, press "Start" on screen, then the "Boot" button.
* When flashing is complete (FINISH appears) then press the Reset button on the ESP32 board (or powercycle ) to start the OpenDTU application.

#### Flash with ESP_Flasher (Windows)

Users report that [ESP_Flasher](https://github.com/Jason2866/ESP_Flasher/releases/) is suitable for flashing OpenDTU on Windows.

#### Flash with [ESP_Flasher](https://espressif.github.io/esptool-js/) - web version

It is also possible to flash it via the web tools which might be more convenient and is platform independent.

## Flashing an Update using "Over The Air" OTA Update

Once you have your OpenDTU running and connected to WLAN, you can do further updates through the web interface.
Navigate to Settings --> Firmware upgrade and press the browse button. Select the firmware file from your local computer.

You'll find the firmware file (after a successful build process) under `.pio/build/generic/firmware.bin`.

If you downloaded a precompiled zip archive, unpack it and choose `opendtu-generic.bin`.

After the successful upload, the OpenDTU immediately restarts into the new firmware.


## Builds

Different builds from existing installations can be found here [Builds](builds/README.md)
Like to show your own build? Just send me a Pull Request.
