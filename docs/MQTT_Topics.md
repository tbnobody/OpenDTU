# MQTT Topics

The base topic, as configured in the web GUI is prepended to all follwing topics.

## General topics

| Topic                                   | R / W | Description                                          | Value / Unit               |
| --------------------------------------- | ----- | ---------------------------------------------------- | -------------------------- |
| dtu/ip                                  | R     | IP address of OpenDTU                                | IP address                 |
| dtu/rssi                                | R     | WiFi network quality                                 | db value                   |
| dtu/status                              | R     | Indicates whether OpenDTU network is reachable       | online /  offline          |
| dtu/uptime                              | R     | Time in seconds since startup                        | seconds                    |

## Inverter specific topics

serial will be replaced with the serial number of the inverter.

| Topic                                   | R / W | Description                                          | Value / Unit               |
| --------------------------------------- | ----- | ---------------------------------------------------- | -------------------------- |
| [serial]/name                           | R     | Name of the inverter as configured in web GUI        |                            |
| [serial]/device/bootloaderversion       | R     | Bootloader version of the inverter                   |                            |
| [serial]/device/fwbuildversion          | R     | Firmware version of the inverter                     |                            |
| [serial]/device/fwbuilddatetime         | R     | Build date / time of inverter firmware               |                            |
| [serial]/device/hwpartnumber            | R     | Hardware part number of the inverter                 |                            |
| [serial]/device/hwversion               | R     | Hardware version of the inverter                     |                            |

### AC channel / global specific topics

| Topic                                   | R / W | Description                                          | Value / Unit               |
| --------------------------------------- | ----- | ---------------------------------------------------- | -------------------------- |
| [serial]/0/current                      | R     | AC current in ampere                                 | Ampere (A)                 |
| [serial]/0/efficiency                   | R     | Ratio AC Power over DC Power in percent              | %                          |
| [serial]/0/frequency                    | R     | AC frequency in hertz                                | Hertz (Hz)                 |
| [serial]/0/power                        | R     | AC active power in watts                             | Watt (W)                   |
| [serial]/0/powerdc                      | R     | DC power in watts                                    | Watt (W)                   |
| [serial]/0/powerfactor                  | R     | Power factor in percent                              | %                          |
| [serial]/0/reactivepower                | R     | AC reactive power in VAr                             | VAr                        |
| [serial]/0/temperature                  | R     | Temperature of inverter in degree celsius            | Degree Celsius (°C)        |
| [serial]/0/voltage                      | R     | AC voltage in volt                                   | Volt (V)                   |
| [serial]/0/yieldday                     | R     | Energy converted to AC per day in watt hours         | Watt hours (Wh)            |
| [serial]/0/yieldtotal                   | R     | Energy converted to AC since reset watt hours        | Watt hours (Wh)            |

### DC input channel topics

[1-4] represents the different inputs. The amount depends on the inverter model.

| Topic                                   | R / W | Description                                          | Value / Unit               |
| --------------------------------------- | ----- | ---------------------------------------------------- | -------------------------- |
| [serial]/[1-4]/current                  | R     | DC current of specific input in ampere               | Ampere (A)                 |
| [serial]/[1-4]/irradiation              | R     | Ratio DC Power over set maximum power (in web GUI)   | %                          |
| [serial]/[1-4]/power                    | R     | DC power of specific input in watt                   | Watt (W)                   |
| [serial]/[1-4]/voltage                  | R     | DC voltage of specific input in volt                 | Volt (V)                   |
| [serial]/[1-4]/yieldday                 | R     | Energy converted to AC per day on specific input     | Watt hours (Wh)            |
| [serial]/[1-4]/yieldtotal               | R     | Energy converted to AC since reset on specific input | Watt hours (Wh)            |

### Inverter limit specific topics

cmd topics are used to set values. Status topics are updated from values set in the inverter.

| Topic                                     | R / W | Description                                          | Value / Unit               |
| ----------------------------------------- | ----- | ---------------------------------------------------- | -------------------------- |
| [serial]/status/limit_relative            | R     | Current applied production limit of the inverter     | % of total possible output |
| [serial]/status/reachable                 | R     | Indicates whether the inverter is reachable          | 0 or 1                     |
| [serial]/status/producing                 | R     | Indicates whether the inverter is producing AC power | 0 or 1                     |
| [serial]/cmd/limit_persistent_relative    | W     | Set the inverter limit as a percentage of total production capability. The  value will survive the night without power | %                          |
| [serial]/cmd/limit_persistent_absolute    | W     | Set the inverter limit as a absolute value. The  value will survive the night without power | Watt (W)                   |
| [serial]/cmd/limit_nonpersistent_relative | W     | Set the inverter limit as a percentage of total production capability. The  value will reset to the last persistent value at night without power | %                          |
| [serial]/cmd/limit_nonpersistent_absolute | W     | Set the inverter limit as a absolute value. The  value will reset to the last persistent value at night without power | Watt (W)                   |