# Web API

Information in JSON format can be obtained through the web API

## List of URLs

may be incomplete

| GET/POST | Auth required | URL |
| -------- | --- | -- |
| Get      | yes | /api/config/get |
| Post     | yes | /api/config/delete |
| Get      | yes | /api/config/list |
| Post     | yes | /api/config/upload |
| Get+Post | yes | /api/device/config |
| Get      | no  | /api/devinfo/status |
| Get+Post | yes | /api/dtu/config |
| Get      | no  | /api/eventlog/status?inv=inverter-serialnumber |
| Post     | yes | /api/firmware/update |
| Get      | yes | /api/inverter/list |
| Post     | yes | /api/inverter/add |
| Post     | yes | /api/inverter/del |
| Post     | yes | /api/inverter/edit |
| Post     | yes | /api/limit/config |
| Get      | no  | /api/limit/status |
| Get      | no  | /api/livedata/status |
| Post     | yes | /api/maintenance/reboot |
| Get+Post | yes | /api/mqtt/config |
| Get      | no  | /api/mqtt/status |
| Get+Post | yes | /api/network/config |
| Get      | no  | /api/network/status |
| Get+Post | yes | /api/ntp/config |
| Get      | no  | /api/ntp/status |
| Get+Post | yes | /api/ntp/time |
| Get      | no  | /api/power/status |
| Post     | yes | /api/power/config |
| Get      | no  | /api/prometheus/metrics |
| Get+Post | yes | /api/security/config |
| Get      | yes | /api/security/authenticate |
| Get      | no  | /api/system/status |

## Examples of Use

### Important notes

- IP addresses and serial numbers in this examples are anonymized. Adjust to your own needs.
- The output from curl is without a linefeed at the end, so please be careful when copying the output - do not accidentally add the shell prompt directly after it.
- When POSTing config data to OpenDTU, always send all settings back, even if only one setting was changed. Sending single settings is not supported and you will receive a response `{"type":"warning","message":"Values are missing!"}`
- When POSTing, always put single quotes around the data part. Do not confuse the single quote `'` with the backtick `` ` ``. You have been warned.
- Some API calls have a single URL for GET and POST - e.g. `/api/ntp/config`
- Other API calls use e.g. `/api/limit/status` to GET data and a different URL `/api/limit/config` to POST data.
- If you want to investigate the web api communication, a good tool is [Postman](https://www.postman.com/)
- Settings API require username and password provided with Basic Authentication credentials
- If you disable the readonly access to the web API, every endpoint requires authentication

### Get information

You can "talk" to the OpenDTU with a command line tool like `curl`. The output is in plain JSON, without carriage return/linefeed and is therefore not very human readable.

#### Get current livedata

```bash
$ curl http://192.168.10.10/api/livedata/status
{"inverters":[{"serial":"11617160xxxx","name":"Meine Solaranlage","data_age":6983,"reachable":false,"producing":false,"limit_relative":0,"limit_absolute":-1,"AC":{"0":{"Power":{"v":0,"u":"W","d":1},"Voltage":{"v":0,"u":"V","d":1},"Current":{"v":0,"u":"A","d":2},"Power DC":{"v":0,"u":"W","d":1},"YieldDay":{"v":0,"u":"Wh","d":0},"YieldTotal":{"v":0,"u":"kWh","d":3},"Frequency":{"v":0,"u":"Hz","d":2},"PowerFactor":{"v":0,"u":"","d":3},"ReactivePower":{"v":0,"u":"var","d":1},"Efficiency":{"v":0,"u":"%","d":3}}},"DC":{"0":{"name":{"u":""},"Power":{"v":0,"u":"W","d":1},"Voltage":{"v":0,"u":"V","d":1},"Current":{"v":0,"u":"A","d":2},"YieldDay":{"v":0,"u":"Wh","d":0},"YieldTotal":{"v":0,"u":"kWh","d":3},"Irradiation":{"v":0,"u":"%","d":3}},"1":{"name":{"u":""},"Power":{"v":0,"u":"W","d":1},"Voltage":{"v":0,"u":"V","d":1},"Current":{"v":0,"u":"A","d":2},"YieldDay":{"v":0,"u":"Wh","d":0},"YieldTotal":{"v":0,"u":"kWh","d":3},"Irradiation":{"v":0,"u":"%","d":3}},"2":{"name":{"u":""},"Power":{"v":0,"u":"W","d":1},"Voltage":{"v":0,"u":"V","d":1},"Current":{"v":0,"u":"A","d":2},"YieldDay":{"v":0,"u":"Wh","d":0},"YieldTotal":{"v":0,"u":"kWh","d":3},"Irradiation":{"v":0,"u":"%","d":3}},"3":{"name":{"u":""},"Power":{"v":0,"u":"W","d":1},"Voltage":{"v":0,"u":"V","d":1},"Current":{"v":0,"u":"A","d":2},"YieldDay":{"v":0,"u":"Wh","d":0},"YieldTotal":{"v":0,"u":"kWh","d":3}}},"INV":{"0":{"Temperature":{"v":0,"u":"°C","d":1}}},"events":0},{"serial":"11417160xxxx","name":"test","data_age":6983,"reachable":false,"producing":false,"limit_relative":0,"limit_absolute":-1,"AC":{"0":{"Power":{"v":0,"u":"W","d":1},"Voltage":{"v":0,"u":"V","d":1},"Current":{"v":0,"u":"A","d":2},"Power DC":{"v":0,"u":"W","d":1},"YieldDay":{"v":0,"u":"Wh","d":0},"YieldTotal":{"v":0,"u":"kWh","d":3},"Frequency":{"v":0,"u":"Hz","d":2},"PowerFactor":{"v":0,"u":"","d":3},"ReactivePower":{"v":0,"u":"var","d":1},"Efficiency":{"v":0,"u":"%","d":3}}},"DC":{"0":{"name":{"u":"test 1"},"Power":{"v":0,"u":"W","d":1},"Voltage":{"v":0,"u":"V","d":1},"Current":{"v":0,"u":"A","d":2},"YieldDay":{"v":0,"u":"Wh","d":0},"YieldTotal":{"v":0,"u":"kWh","d":3},"Irradiation":{"v":0,"u":"%","d":3}},"1":{"name":{"u":"test 2"},"Power":{"v":0,"u":"W","d":1},"Voltage":{"v":0,"u":"V","d":1},"Current":{"v":0,"u":"A","d":2},"YieldDay":{"v":0,"u":"Wh","d":0},"YieldTotal":{"v":0,"u":"kWh","d":3},"Irradiation":{"v":0,"u":"%","d":3}}},"INV":{"0":{"Temperature":{"v":0,"u":"°C","d":1}}},"events":0}],"total":{"Power":{"v":0,"u":"W","d":1},"YieldDay":{"v":0,"u":"Wh","d":0},"YieldTotal":{"v":0,"u":"kWh","d":2}},"hints":{"time_sync":false,"radio_problem":false,"default_password":false}}
```

To enhance readability (and filter information) use the JSON command line processor `jq`.

```bash
$ curl --no-progress-meter http://192.168.10.10/api/livedata/status | jq
{
  "inverters": [
    {
      "serial": "116171603546",
      "name": "Meine Solaranlage",
      "data_age": 7038,
      "reachable": false,
      "producing": false,
      "limit_relative": 0,
      "limit_absolute": -1,
      "AC": {
        "0": {
          "Power": {
            "v": 0,
            "u": "W",
            "d": 1
          },
          "Voltage": {
            "v": 0,
            "u": "V",
            "d": 1
          },
          "Current": {
            "v": 0,
            "u": "A",
            "d": 2
          },
          "Power DC": {
            "v": 0,
            "u": "W",
            "d": 1
          },
          "YieldDay": {
            "v": 0,
            "u": "Wh",
            "d": 0
          },
          "YieldTotal": {
            "v": 0,
            "u": "kWh",
            "d": 3
          },
          "Frequency": {
            "v": 0,
            "u": "Hz",
            "d": 2
          },
          "PowerFactor": {
            "v": 0,
            "u": "",
            "d": 3
          },
          "ReactivePower": {
            "v": 0,
            "u": "var",
            "d": 1
          },
          "Efficiency": {
            "v": 0,
            "u": "%",
            "d": 3
          }
        }
      },
      "DC": {
        "0": {
          "name": {
            "u": ""
          },
          "Power": {
            "v": 0,
            "u": "W",
            "d": 1
          },
          "Voltage": {
            "v": 0,
            "u": "V",
            "d": 1
          },
          "Current": {
            "v": 0,
            "u": "A",
            "d": 2
          },
          "YieldDay": {
            "v": 0,
            "u": "Wh",
            "d": 0
          },
          "YieldTotal": {
            "v": 0,
            "u": "kWh",
            "d": 3
          },
          "Irradiation": {
            "v": 0,
            "u": "%",
            "d": 3
          }
        },
        "1": {
          "name": {
            "u": ""
          },
          "Power": {
            "v": 0,
            "u": "W",
            "d": 1
          },
          "Voltage": {
            "v": 0,
            "u": "V",
            "d": 1
          },
          "Current": {
            "v": 0,
            "u": "A",
            "d": 2
          },
          "YieldDay": {
            "v": 0,
            "u": "Wh",
            "d": 0
          },
          "YieldTotal": {
            "v": 0,
            "u": "kWh",
            "d": 3
          },
          "Irradiation": {
            "v": 0,
            "u": "%",
            "d": 3
          }
        },
        "2": {
          "name": {
            "u": ""
          },
          "Power": {
            "v": 0,
            "u": "W",
            "d": 1
          },
          "Voltage": {
            "v": 0,
            "u": "V",
            "d": 1
          },
          "Current": {
            "v": 0,
            "u": "A",
            "d": 2
          },
          "YieldDay": {
            "v": 0,
            "u": "Wh",
            "d": 0
          },
          "YieldTotal": {
            "v": 0,
            "u": "kWh",
            "d": 3
          },
          "Irradiation": {
            "v": 0,
            "u": "%",
            "d": 3
          }
        },
        "3": {
          "name": {
            "u": ""
          },
          "Power": {
            "v": 0,
            "u": "W",
            "d": 1
          },
          "Voltage": {
            "v": 0,
            "u": "V",
            "d": 1
          },
          "Current": {
            "v": 0,
            "u": "A",
            "d": 2
          },
          "YieldDay": {
            "v": 0,
            "u": "Wh",
            "d": 0
          },
          "YieldTotal": {
            "v": 0,
            "u": "kWh",
            "d": 3
          }
        }
      },
      "INV": {
        "0": {
          "Temperature": {
            "v": 0,
            "u": "°C",
            "d": 1
          }
        }
      },
      "events": 0
    },
    {
      "serial": "114171603548",
      "name": "test",
      "data_age": 7038,
      "reachable": false,
      "producing": false,
      "limit_relative": 0,
      "limit_absolute": -1,
      "AC": {
        "0": {
          "Power": {
            "v": 0,
            "u": "W",
            "d": 1
          },
          "Voltage": {
            "v": 0,
            "u": "V",
            "d": 1
          },
          "Current": {
            "v": 0,
            "u": "A",
            "d": 2
          },
          "Power DC": {
            "v": 0,
            "u": "W",
            "d": 1
          },
          "YieldDay": {
            "v": 0,
            "u": "Wh",
            "d": 0
          },
          "YieldTotal": {
            "v": 0,
            "u": "kWh",
            "d": 3
          },
          "Frequency": {
            "v": 0,
            "u": "Hz",
            "d": 2
          },
          "PowerFactor": {
            "v": 0,
            "u": "",
            "d": 3
          },
          "ReactivePower": {
            "v": 0,
            "u": "var",
            "d": 1
          },
          "Efficiency": {
            "v": 0,
            "u": "%",
            "d": 3
          }
        }
      },
      "DC": {
        "0": {
          "name": {
            "u": "test 1"
          },
          "Power": {
            "v": 0,
            "u": "W",
            "d": 1
          },
          "Voltage": {
            "v": 0,
            "u": "V",
            "d": 1
          },
          "Current": {
            "v": 0,
            "u": "A",
            "d": 2
          },
          "YieldDay": {
            "v": 0,
            "u": "Wh",
            "d": 0
          },
          "YieldTotal": {
            "v": 0,
            "u": "kWh",
            "d": 3
          },
          "Irradiation": {
            "v": 0,
            "u": "%",
            "d": 3
          }
        },
        "1": {
          "name": {
            "u": "test 2"
          },
          "Power": {
            "v": 0,
            "u": "W",
            "d": 1
          },
          "Voltage": {
            "v": 0,
            "u": "V",
            "d": 1
          },
          "Current": {
            "v": 0,
            "u": "A",
            "d": 2
          },
          "YieldDay": {
            "v": 0,
            "u": "Wh",
            "d": 0
          },
          "YieldTotal": {
            "v": 0,
            "u": "kWh",
            "d": 3
          },
          "Irradiation": {
            "v": 0,
            "u": "%",
            "d": 3
          }
        }
      },
      "INV": {
        "0": {
          "Temperature": {
            "v": 0,
            "u": "°C",
            "d": 1
          }
        }
      },
      "events": 0
    }
  ],
  "total": {
    "Power": {
      "v": 0,
      "u": "W",
      "d": 1
    },
    "YieldDay": {
      "v": 0,
      "u": "Wh",
      "d": 0
    },
    "YieldTotal": {
      "v": 0,
      "u": "kWh",
      "d": 2
    }
  },
  "hints": {
    "time_sync": false,
    "radio_problem": false,
    "default_password": false
  }
}
```

The eventlog can be fetched with the inverter serial number as parameter:

```bash
$ curl --no-progress-meter http://192.168.10.10/api/eventlog/status?inv=11418186xxxx | jq
{
  "11418186xxxx": {
    "count": 4,
    "events": [
      {
        "message_id": 1,
        "message": "Inverter start",
        "start_time": 28028,
        "end_time": 28028
      },
      {
        "message_id": 209,
        "message": "PV-1: No input",
        "start_time": 28036,
        "end_time": 0
      },
      {
        "message_id": 2,
        "message": "DTU command failed",
        "start_time": 28092,
        "end_time": 28092
      },
      {
        "message_id": 207,
        "message": "MPPT-A: Input undervoltage",
        "start_time": 28336,
        "end_time": 0
      }
    ]
  }
}
```

#### combine curl and jq

`jq` can filter specific fields from json output.

For example, filter out the current total power:

```bash
$ curl --no-progress-meter http://192.168.10.10/api/livedata/status | jq '.total | .Power.v'
140.7999878
```

#### Get information where login is required

When config data is requested, username and password have to be provided to `curl`
Username is always `admin`, the default password is `openDTU42`. The password is used for both the admin login and the Admin-mode Access Point.

```bash
$ curl --u admin:openDTU42 http://192.168.10.10/api/ntp/config
{"ntp_server":"pool.ntp.org","ntp_timezone":"CET-1CEST,M3.5.0,M10.5.0/3","ntp_timezone_descr":"Europe/Berlin"}
```

### Post information

With HTTP POST commands information can be written to the OpenDTU.

The Web API is designed to allow the web frontend in the web browser to communicate with the OpenDTU software running on the ESP32. It is not designed to be intuitive or user-friendly, so please follow the instructions here.

#### Example 1: change ntp settings

If you want to configure the ntp server setting, first fetch the information from the web API:

```bash
$ curl -u "admin:password" http://192.168.10.10/api/ntp/config
{"ntp_server":"pool.ntp.org","ntp_timezone":"CET-1CEST,M3.5.0,M10.5.0/3","ntp_timezone_descr":"Europe/Berlin"}
```

Then, second step, send your new settings. Use the text output from curl in the first step, add `data=` and enclose the whole data with single quotes.

```bash
$ curl -u "admin:password" http://192.168.10.10/api/ntp/config -d 'data={"ntp_server":"my.own.ntp.server.home","ntp_timezone":"CET-1CEST,M3.5.0,M10.5.0/3","ntp_timezone_descr":"Europe/Berlin"}'
{"type":"success","message":"Settings saved!"}
```

You will receive a json formatted response.

#### Example 2: change power limit

In the second example, I want to change the non persistent power limit of an inverter. Again, first fetch current data:

```bash
$ curl http://192.168.10.10/api/limit/status
{"11418186xxxx":{"limit_relative":100,"max_power":600,"limit_set_status":"Ok"},"11418180xxxx":{"limit_relative":100,"max_power":800,"limit_set_status":"Ok"}}
```

I see data from two configured inverters.

Now I set the relative power limit of inverter with serialnumber `11418180xxxx` to 50%.

```bash
$ curl -u "admin:password" http://192.168.10.10/api/limit/config -d 'data={"serial":"11418180xxxx", "limit_type":1, "limit_value":50}'
{"type":"success","message":"Settings saved!"}
```

Then I read again the limit status. In the first answer the status is `pending`, some seconds later it changed to `OK`.

```bash
$ curl http://192.168.10.10/api/limit/status
{"11418186xxxx":{"limit_relative":100,"max_power":600,"limit_set_status":"Ok"},"11418180xxxx":{"limit_relative":100,"max_power":800,"limit_set_status":"Pending"}}

...

$ curl http://192.168.10.10/api/limit/status
{"11418186xxxx":{"limit_relative":100,"max_power":600,"limit_set_status":"Ok"},"11418180xxxx":{"limit_relative":50,"max_power":800,"limit_set_status":"Ok"}}
```
