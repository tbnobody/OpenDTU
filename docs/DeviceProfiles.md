# Device Profiles

This documentation will has been moved and can be found here: <https://tbnobody.github.io/OpenDTU-docs/firmware/device_profiles/>

## Structure of the json file for openDTU-OnBattery (outdated example)

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
        "victron": {
            "rx": 22,
            "tx": 21
        },
        "battery": {
            "rx": 27,
            "tx": 14
        },
        "huawei": {
            "miso": 12,
            "mosi": 13,
            "clk": 26,
            "irq": 25,
            "power": 33,
            "cs": 15
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