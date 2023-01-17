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

The json file can contain multiple profiles. Each profile requires a name and different parameters. If one parameter is not set, the default value, as compiled into the firmware is used. The example above shows all the currently supported values. Others may follow.