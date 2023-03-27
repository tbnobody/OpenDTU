# Upgrade Partition

To be able to install further updates you have to update the partition table of the ESP32. Doing so will **erase** all configuration data. Over The Air update using the web interface is **NOT** possible!

**So make sure you export a backup of your configuration files before continuing.**

There are several possibilities to update the partition table:
- Using Visual Studio Code or PlatformIO CLI

   If you have already used Visual Studio Code or the `platformio` command you can use it again to install the latest version. The partition table is upgraded automatically.

- Any kind of flash interface

   If you like to use any kind of flash interface like `esptool.py`, Espressif Flash Download Tool, ESP_Flasher or esptool-js you have to make sure to upload **ALL** provided .bin files. It is important to enter the correct target addresses.

   | Address  | File                   |
   | ---------| ---------------------- |
   | 0x1000   | bootloader.bin         |
   | 0x8000   | partitions.bin         |
   | 0xe000   | boot_app0.bin          |
   | 0x10000  | opendtu-*.bin          |


After upgrading the ESP32 will open the intergrated access point (AP) again. Just connect to it using the default password ("openDTU42"). If you are connected, just visit http://192.168.4.1 and enter the "Configuration Management". Recover the previously backuped config files.