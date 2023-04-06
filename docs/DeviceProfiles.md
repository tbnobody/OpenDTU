# Device Profiles

It is possible to change hardware settings like pin assignments or ethernet support using a json file. The json file can be uploaded using the configuration management in the web interface. Just select "Pin Mapping (pin_mapping.json)" in the recovery section.

When the file is uploaded the ESP performs a reboot. This is required as the pin settings could have changed within the file. By default all the pin assignments are used as compiled into the firmware.

To change the device profile, navigate to the "Device Manager" and selected the appropriate profile. You can see the current (Active) and the new (Selected) in assignment in the table below the combobox.

## Structure of the json file

```json
[
    {
        "name": "Generic NodeMCU 38 pin",
        "nrf24": {
            "miso": 19,
            "mosi": 23,
            "clk": 18,
            "irq": 16,
            "en": 4,
            "cs": 5
        },
        "eth": {
            "enabled": false,
            "phy_addr": -1,
            "power": -1,
            "mdc": -1,
            "mdio": -1,
            "type": -1,
            "clk_mode": -1
        }
    },
    {
        "name": "Generic NodeMCU 38 pin with SSD1306",
        "nrf24": {
            "miso": 19,
            "mosi": 23,
            "clk": 18,
            "irq": 16,
            "en": 4,
            "cs": 5
        },
        "eth": {
            "enabled": false,
            "phy_addr": -1,
            "power": -1,
            "mdc": -1,
            "mdio": -1,
            "type": -1,
            "clk_mode": -1
        },
        "display": {
            "type": 2,
            "data": 21,
            "clk": 22
        }
    },
    {
        "name": "Olimex ESP32-POE",
        "nrf24": {
            "miso": 15,
            "mosi": 2,
            "clk": 14,
            "irq": 13,
            "en": 16,
            "cs": 5
        },
        "eth": {
            "enabled": true,
            "phy_addr": 0,
            "power": 12,
            "mdc": 23,
            "mdio": 18,
            "type": 0,
            "clk_mode": 3
        }
    }
]
```

The json file can contain multiple profiles. Each profile requires a name and different parameters. If one parameter is not set, the default value, as compiled into the firmware is used. The example above shows all the currently supported values. Others may follow. Sample files for some boards can be found [here](DeviceProfiles/). This means you can just flash the generic bin file and upload the json file. Then you select your board and everything works hopyfully as expected.

## Implemented configuration values

| Parameter     | Data Type | Description |
| ------------- | --------- | ----------- |
| name          | string    | Unique name of the profile (max 63 characters) |
| nrf24.miso    | number    | MISO Pin |
| nrf24.mosi    | number    | MOSI Pin |
| nrf24.clk     | number    | Clock Pin |
| nrf24.irq     | number    | Interrupt Pin |
| nrf24.en      | number    | Enable Pin |
| nrf24.cs      | number    | Chip Select Pin |
| eth.enabled   | boolean   | Enable/Disable the ethernet stack |
| eth.phy_addr  | number    | Unique PHY addr |
| eth.power     | number    | Power Pin (if available). Use -1 for not assigned pins. |
| eth.mdc       | number    | Serial Management Interface MDC Pin. Use -1 for not assigned pins. |
| eth.mdio      | number    | Serial Management Interface MDIO Pin. Use -1 for not assigned pins. |
| eth.type      | number    | Possible values:<br>* 0 = ETH_PHY_LAN8720<br>* 1 = ETH_PHY_TLK110<br>* 2 = ETH_PHY_RTL8201<br>* 3 = ETH_PHY_DP83848<br>* 4 = ETH_PHY_DM9051<br>* 5 = ETH_PHY_KSZ8041<br>* 6 = ETH_PHY_KSZ8081 |
| eth.clk_mode  | number    | Possible values:<br>* 0 = ETH_CLOCK_GPIO0_IN<br>* 1 = ETH_CLOCK_GPIO0_OUT<br>* 2 = ETH_CLOCK_GPIO16_OUT<br>* 3 = ETH_CLOCK_GPIO17_OUT |
| display.type  | number    | Specify type of display. Possible values:<br>* 0 = None (default)<br>* 1 = PCD8544 <br>* 2 = SSD1306 <br>* 3 = SH1106 |
| display.data  | number    | Data Pin (e.g. SDA for i2c displays) required for all displays. Use 255 for not assigned pins. |
| display.clk   | number    | Clock Pin (e.g. SCL for i2c displays) required for SSD1306 and SH1106. Use 255 for not assigned pins. |
| display.cs    | number    | Chip Select Pin required for PCD8544. Use 255 for not assigned pins. |
| display.reset | number    | Reset Pin required for PCD8544, optional for all other displays. Use 255 for not assigned pins. |
| led.led0      | number    | LED pin for network indication. Blinking = WLAN connected but NTP & MQTT (if enabled) disconnected. On = WLAN, NTP, MQTT connected. Off = Network not connected |
| led.led1      | number    | LED pin for inverter indication. On = All inverters reachable & producing. Blinking = All inverters reachable but not producing. Off = At least one inverter is not reachable. Only inverters with polling enabled are considered. |