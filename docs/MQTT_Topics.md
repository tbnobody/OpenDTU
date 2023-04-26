# MQTT Topics

The base topic, as configured in the web GUI is prepended to all following topics.
Serial will be replaced with the serial number of the respective device.

## General topics

| Topic                                   | R / W | Description                                          | Value / Unit               |
| --------------------------------------- | ----- | ---------------------------------------------------- | -------------------------- |
| dtu/ip                                  | R     | IP address of OpenDTU                                | IP address                 |
| dtu/hostname                            | R     | Current hostname of the dtu (as set in web GUI)      |                            |
| dtu/rssi                                | R     | WiFi network quality                                 | db value                   |
| dtu/status                              | R     | Indicates whether OpenDTU network is reachable       | online /  offline          |
| dtu/uptime                              | R     | Time in seconds since startup                        | seconds                    |

## Inverter total topicss

Enabled inverter means, that only inverters with "Poll inverter data" enabled are considered.

| Topic                                   | R / W | Description                                          | Value / Unit               |
| --------------------------------------- | ----- | ---------------------------------------------------- | -------------------------- |
| ac/power                                | R     | Sum of AC active power of all enabled inverters      | W                          |
| ac/yieldtotal                           | R     | Sum of energy converted to AC since reset watt hours of all enabled inverters | Kilo watt hours (kWh) |
| ac/yieldday                             | R     | Sum of energy converted to AC per day in watt hours of all enabled inverters | Watt hours (Wh)
| ac/is_valid                             | R     | Indicator whether all enabled inverters where reachable | 0 or 1                  |
| dc/power                                | R     | Sum of DC power of all enabled inverters             | Watt (W)                   |
| dc/irradiation                          | R     | Produced power of all enabled inverter stripes with defined irradiation settings divided by sum of all enabled inverters irradiation | % |
| dc/is_valid                             | R     | Indicator whether all enabled inverters where reachable | 0 or 1                  |

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
| [serial]/status/reachable               | R     | Indicates whether the inverter is reachable          | 0 or 1                     |
| [serial]/status/producing               | R     | Indicates whether the inverter is producing AC power | 0 or 1                     |
| [serial]/status/last_update             | R     | Unix timestamp of last inverter statistics udpate    | seconds since JAN 01 1970 (UTC) |

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
| [serial]/0/yieldtotal                   | R     | Energy converted to AC since reset watt hours        | Kilo watt hours (kWh)      |

### DC input channel topics

[1-4] represents the different inputs. The amount depends on the inverter model.

| Topic                                   | R / W | Description                                          | Value / Unit               |
| --------------------------------------- | ----- | ---------------------------------------------------- | -------------------------- |
| [serial]/[1-4]/current                  | R     | DC current of specific input in ampere               | Ampere (A)                 |
| [serial]/[1-4]/name                     | R     | Name of the DC input channel as configured in web GUI|                            |
| [serial]/[1-4]/irradiation              | R     | Ratio DC Power over set maximum power (in web GUI)   | %                          |
| [serial]/[1-4]/power                    | R     | DC power of specific input in watt                   | Watt (W)                   |
| [serial]/[1-4]/voltage                  | R     | DC voltage of specific input in volt                 | Volt (V)                   |
| [serial]/[1-4]/yieldday                 | R     | Energy converted to AC per day on specific input     | Watt hours (Wh)            |
| [serial]/[1-4]/yieldtotal               | R     | Energy converted to AC since reset on specific input | Kilo watt hours (kWh)      |

### Inverter limit specific topics

cmd topics are used to set values. Status topics are updated from values set in the inverter.

| Topic                                     | R / W | Description                                          | Value / Unit               |
| ----------------------------------------- | ----- | ---------------------------------------------------- | -------------------------- |
| [serial]/status/limit_relative            | R     | Current applied production limit of the inverter     | % of total possible output |
| [serial]/status/limit_absolute            | R     | Current applied production limit of the inverter     | Watt (W)                   |
| [serial]/cmd/limit_persistent_relative    | W     | Set the inverter limit as a percentage of total production capability. The  value will survive the night without power. The updated value will show up in the web GUI and limit_relative topic immediatly. | %                          |
| [serial]/cmd/limit_persistent_absolute    | W     | Set the inverter limit as a absolute value. The  value will survive the night without power. The updated value will set immediatly within the inverter but show up in the web GUI and limit_relative topic after around 4 minutes. If you are using a already known inverter (known Hardware ID), the updated value will show up within a few seconds. | Watt (W)                   |
| [serial]/cmd/limit_nonpersistent_relative | W     | Set the inverter limit as a percentage of total production capability. The  value will reset to the last persistent value at night without power. The updated value will show up in the web GUI and limit_relative topic immediatly. The value must be published non-retained, otherwise it will be ignored! | %                          |
| [serial]/cmd/limit_nonpersistent_absolute | W     | Set the inverter limit as a absolute value. The  value will reset to the last persistent value at night without power. The updated value will set immediatly within the inverter but show up in the web GUI and limit_relative topic after around 4 minutes. If you are using a already known inverter (known Hardware ID), the updated value will show up within a few seconds. The value must be published non-retained, otherwise it will be ignored! | Watt (W)                   |
| [serial]/cmd/power                        | W      | Turn the inverter on (1) or off (0)                 | 0 or 1                     |
| [serial]/cmd/restart                      | W      | Restarts the inverters (also resets YieldDay)       | 1                          |

### Victron MPPT topics

#### General

| Topic                                   | R / W | Description                                          | Value / Unit               |
| --------------------------------------- | ----- | ---------------------------------------------------- | -------------------------- |
| victron/[serial]/PID                    | R     | Product description                                  | text                       |
| victron/[serial]/SER                    | R     | Serial number                                        | text                       |
| victron/[serial]/FW                     | R     | Firmware number                                      | int                        |
| victron/[serial]/LOAD                   | R     | Load output state                                    | ON /  OFF                  |
| victron/[serial]/CS                     | R     | State of operation                                   | text e. g. "Bulk"          |
| victron/[serial]/ERR                    | R     | Error code                                           | text e. g. "No error"      |
| victron/[serial]/OR                     | R     | Off reasen                                           | text e. g. "Not off"       |
| victron/[serial]/MPPT                   | R     | Tracker operation mode                               | text e. g. "MPP Tracker active" |
| victron/[serial]/HSDS                   | R     | Day sequence number (0...364)                        | int in days                |

#### Battery output

| Topic                                   | R / W | Description                                          | Value / Unit               |
| --------------------------------------- | ----- | ---------------------------------------------------- | -------------------------- |
| victron/[serial]/V                      | R     | Voltage                                              | Volt (V)                   |
| victron/[serial]/I                      | R     | Current                                              | Ampere (A)                 |

#### Solar input

| Topic                                   | R / W | Description                                          | Value / Unit               |
| --------------------------------------- | ----- | ---------------------------------------------------- | -------------------------- |
| victron/[serial]/VPV                    | R     | Voltage                                              | Volt (V)                   |
| victron/[serial]/PPV                    | R     | Power                                                | Watt (W)                   |
| victron/[serial]/H19                    | R     | Yield total (user resettable counter)                | Kilo watt hours (kWh)      |
| victron/[serial]/H20                    | R     | Yield today                                          | Kilo watt hours (kWh)      |
| victron/[serial]/H21                    | R     | Maximum power today                                  | Watt (W)                   |
| victron/[serial]/H22                    | R     | Yield yesterday                                      | Kilo watt hours (kWh)      |
| victron/[serial]/H23                    | R     | Maximum power yesterday                              | Watt (W)                   |

### Pylontech battery topics

| Topic                                   | R / W | Description                                          | Value / Unit               |
| --------------------------------------- | ----- | ---------------------------------------------------- | -------------------------- |
| battery/settings/chargeVoltage          | R     | Voltage                                              | Volt (V)                   |
| battery/settings/chargeCurrentLimitation | R    | BMS requested max. charge current                    | Ampere (A)                 |
| battery/settings/dischargeCurrentLimitation | R | BMS requested max. discharge current                 | Ampere (A)                 |
| battery/stateOfCharge                   | R     | State of Health                                      | %                          |
| battery/stateOfHealth                   | R     | State of Charge                                      | %                          |
| battery/voltage                         | R     | Actual voltage                                       | Volt (V)                   |
| battery/current                         | R     | Actual current                                       | Ampere (A)                 |
| battery/temperature"                    | R     | Actual temperature                                   | °C                         |
| battery/alarm/overCurrentDischarge      | R     | Alarm: High discharge current                        | 0 / 1                      |
| battery/alarm/underTemperature          | R     | Alarm: Low temperature                               | 0 / 1                      |
| battery/alarm/overTemperature           | R     | Alarm: High temperature                              | 0 / 1                      |
| battery/alarm/underVoltage              | R     | Alarm: Low voltage                                   | 0 / 1                      |
| battery/alarm/overVoltage               | R     | Alarm: High voltage                                  | 0 / 1                      |
| battery/alarm/bmsInternal               | R     | Alarm: BMS internal                                  | 0 / 1                      |
| battery/warning/highCurrentDischarge    | R     | Warning: High discharge current                      | 0 / 1                      |
| battery/warning/lowTemperature          | R     | Warning: Low temperature                             | 0 / 1                      |
| battery/warning/highTemperature         | R     | Warning: High temperature                            | 0 / 1                      |
| battery/warning/lowVoltage              | R     | Warning: Low voltage                                 | 0 / 1                      |
| battery/warning/highVoltage             | R     | Warning: High voltage                                | 0 / 1                      |
| battery/warning/bmsInternal             | R     | Warning: BMS internal                                | 0 / 1                      |
| battery/manufacturer                    | R     | Manufacturer                                         | String                     |
| battery/charging/chargeEnabled          | R     | Charge enabled flag                                  | 0 / 1                      |
| battery/charging/dischargeEnabled       | R     | Discharge enabled flag                               | 0 / 1                      |
| battery/charging/chargeImmediately      | R     | Charge immediately flag                              | 0 / 1                      |

### Huawei AC charger topics
| Topic                                   | R / W | Description                                          | Value / Unit               |
| --------------------------------------- | ----- | ---------------------------------------------------- | -------------------------- |
| huawei/cmd/limit_online_voltage         | W     | Online voltage (i.e. CAN bus connected)              | Volt (V)                   |
| huawei/cmd/limit_online_current         | W     | Online current (i.e. CAN bus connected)              | Ampere (A)                 |
| huawei/cmd/power                        | W     | Controls output pin GPIO to drive solid state relais | 0 / 1                      |
| huawei/data_age                         | R     | How old the data is                                  | Seconds                    |
| huawei/input_voltage                    | R     | Input voltage                                        | Volt (V)                   |
| huawei/input_current                    | R     | Input current                                        | Ampere (A)                 |
| huawei/input_power                      | R     | Input power                                          | Watt (W)                   |
| huawei/output_voltage                   | R     | Output voltage                                       | Volt (V)                   |
| huawei/output_current                   | R     | Output current                                       | Ampere (A)                 |
| huawei/max_output_current               | R     | Maximum output current (set using the online limit)  | Ampere (A)                 |
| huawei/output_power                     | R     | Output power                                         | Watt (W)                   |
| huawei/input_temp                       | R     | Input air temperature                                | °C                         |
| huawei/output_temp                      | R     | Output air temperature                               | °C                         |
| huawei/efficiency                       | R     | Efficiency                                           | Percentage                 |