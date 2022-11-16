# Web API

Information in JSON format can be obtained through the web API

## List of URLs
may be incomplete

| GET/POST | Auth required | URL |
| -------- | --- | -- |
| Get      | yes | /api/config/get |
| Post     | yes | /api/config/delete |
| Post     | yes | /api/config/upload |
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
| Get+Post | yes | /api/mqtt/config |
| Get      | no  | /api/mqtt/status |
| Get+Post | yes | /api/network/config |
| Get      | no  | /api/network/status |
| Get+Post | yes | /api/ntp/config |
| Get      | no  | /api/ntp/status |
| Get+Post | yes | /api/ntp/time |
| Get      | no  | /api/power/status |
| Post     | yes | /api/power/config |
| Get+Post | yes | /api/security/password |
| Get      | no  | /api/system/status |


## Examples of Use

### Important notes:

- IP addresses and serial numbers in this examples are anonymized. Adjust to your own needs.
- The output from curl is without a linefeed at the end, so please be careful when copying the output - do not accidentally add the shell prompt directly after it.
- When POSTing config data to OpenDTU, always send all settings back, even if only one setting was changed. Sending single settings is not supported and you will receive a response `{"type":"warning","message":"Values are missing!"}`
- When POSTing, always put single quotes around the data part. Do not confuse the single quote `'` with the backtick `` ` ``. You have been warned.
- Some API calls have a single URL for GET and POST - e.g. `/api/ntp/config`
- Other API calls use e.g. `/api/limit/status` to GET data and a different URL `/api/limit/config` to POST data.
- If you want to investigate the web api communication, a good tool is [Postman](https://www.postman.com/)
- Settings API require username and password provided with Basic Authentication credentials


### Get information

You can "talk" to the OpenDTU with a command line tool like `curl`. The output is in plain JSON, without carriage return/linefeed and is therefore not very human readable.

#### Get current livedata

```
~$ curl http://192.168.10.10/api/livedata/status
{"inverters":[{"serial":"11418186xxxx","name":"HM600","data_age":4,"reachable":true,"producing":true,"limit_relative":100,"limit_absolute":600,"0":{"Power":{"v":70.69999695,"u":"W","d":1},"Voltage":{"v":233,"u":"V","d":1},"Current":{"v":0.300000012,"u":"A","d":2},"Power DC":{"v":74,"u":"W","d":2},"YieldDay":{"v":23,"u":"Wh","d":2},"YieldTotal":{"v":150.5050049,"u":"kWh","d":2},"Frequency":{"v":50.02000046,"u":"Hz","d":2},"Temperature":{"v":8.300000191,"u":"°C","d":1},"PowerFactor":{"v":1,"u":"","d":3},"ReactivePower":{"v":0.100000001,"u":"var","d":1},"Efficiency":{"v":95.54053497,"u":"%","d":2}},"1":{"Power":{"v":0,"u":"W","d":1},"Voltage":{"v":1,"u":"V","d":1},"Current":{"v":0.02,"u":"A","d":2},"YieldDay":{"v":0,"u":"Wh","d":0},"YieldTotal":{"v":49.0320015,"u":"kWh","d":3},"Irradiation":{"v":0,"u":"%","d":2}},"2":{"Power":{"v":74,"u":"W","d":1},"Voltage":{"v":42.40000153,"u":"V","d":1},"Current":{"v":1.74000001,"u":"A","d":2},"YieldDay":{"v":23,"u":"Wh","d":0},"YieldTotal":{"v":101.4729996,"u":"kWh","d":3},"Irradiation":{"v":18.04878044,"u":"%","d":2}},"events":3},{"serial":"11418180xxxx","name":"HM800","data_age":11,"reachable":true,"producing":true,"limit_relative":100,"limit_absolute":800,"0":{"Power":{"v":70.09999847,"u":"W","d":1},"Voltage":{"v":233.1000061,"u":"V","d":1},"Current":{"v":0.300000012,"u":"A","d":2},"Power DC":{"v":73.59999847,"u":"W","d":2},"YieldDay":{"v":48,"u":"Wh","d":2},"YieldTotal":{"v":48.5399971,"u":"kWh","d":2},"Frequency":{"v":50.02000046,"u":"Hz","d":2},"Temperature":{"v":11.39999962,"u":"°C","d":1},"PowerFactor":{"v":1,"u":"","d":3},"ReactivePower":{"v":0.100000001,"u":"var","d":1},"Efficiency":{"v":95.24456024,"u":"%","d":2}},"1":{"Power":{"v":36.5,"u":"W","d":1},"Voltage":{"v":39.09999847,"u":"V","d":1},"Current":{"v":0.930000007,"u":"A","d":2},"YieldDay":{"v":31,"u":"Wh","d":0},"YieldTotal":{"v":4.301000118,"u":"kWh","d":3},"Irradiation":{"v":8.902439117,"u":"%","d":2}},"2":{"Power":{"v":37.09999847,"u":"W","d":1},"Voltage":{"v":40.79999924,"u":"V","d":1},"Current":{"v":0.910000026,"u":"A","d":2},"YieldDay":{"v":17,"u":"Wh","d":0},"YieldTotal":{"v":44.23899841,"u":"kWh","d":3},"Irradiation":{"v":9.048780441,"u":"%","d":2}},"events":1}],"total":{"Power":{"v":140.7999878,"u":"W","d":1},"YieldDay":{"v":71,"u":"Wh","d":0},"YieldTotal":{"v":199.0449982,"u":"kWh","d":2}}}
```


To enhance readability (and filter information) use the JSON command line processor `jq`.

```
~$ curl --no-progress-meter http://192.168.10.10/api/livedata/status | jq
{
  "inverters": [
    {
      "serial": "11418186xxxx",
      "name": "HM600",
      "data_age": 4,
      "reachable": true,
      "producing": true,
      "limit_relative": 100,
      "limit_absolute": 600,
      "0": {
        "Power": {
          "v": 70.69999695,
          "u": "W",
          "d": 1
        },
        "Voltage": {
          "v": 233,
          "u": "V",
          "d": 1
        },
        "Current": {
          "v": 0.300000012,
          "u": "A",
          "d": 2
        },
        "Power DC": {
          "v": 74,
          "u": "W",
          "d": 2
        },
        "YieldDay": {
          "v": 23,
          "u": "Wh",
          "d": 2
        },
        "YieldTotal": {
          "v": 150.5050049,
          "u": "kWh",
          "d": 2
        },
        "Frequency": {
          "v": 50.02000046,
          "u": "Hz",
          "d": 2
        },
        "Temperature": {
          "v": 8.300000191,
          "u": "°C",
          "d": 1
        },
        "PowerFactor": {
          "v": 1,
          "u": "",
          "d": 3
        },
        "ReactivePower": {
          "v": 0.100000001,
          "u": "var",
          "d": 1
        },
        "Efficiency": {
          "v": 95.54053497,
          "u": "%",
          "d": 2
        }
      },
      "1": {
        "Power": {
          "v": 0,
          "u": "W",
          "d": 1
        },
        "Voltage": {
          "v": 1,
          "u": "V",
          "d": 1
        },
        "Current": {
          "v": 0.02,
          "u": "A",
          "d": 2
        },
        "YieldDay": {
          "v": 0,
          "u": "Wh",
          "d": 0
        },
        "YieldTotal": {
          "v": 49.0320015,
          "u": "kWh",
          "d": 3
        },
        "Irradiation": {
          "v": 0,
          "u": "%",
          "d": 2
        }
      },
      "2": {
        "Power": {
          "v": 74,
          "u": "W",
          "d": 1
        },
        "Voltage": {
          "v": 42.40000153,
          "u": "V",
          "d": 1
        },
        "Current": {
          "v": 1.74000001,
          "u": "A",
          "d": 2
        },
        "YieldDay": {
          "v": 23,
          "u": "Wh",
          "d": 0
        },
        "YieldTotal": {
          "v": 101.4729996,
          "u": "kWh",
          "d": 3
        },
        "Irradiation": {
          "v": 18.04878044,
          "u": "%",
          "d": 2
        }
      },
      "events": 3
    },
    {
      "serial": "11418180xxxx",
      "name": "HM800",
      "data_age": 11,
      "reachable": true,
      "producing": true,
      "limit_relative": 100,
      "limit_absolute": 800,
      "0": {
        "Power": {
          "v": 70.09999847,
          "u": "W",
          "d": 1
        },
        "Voltage": {
          "v": 233.1000061,
          "u": "V",
          "d": 1
        },
        "Current": {
          "v": 0.300000012,
          "u": "A",
          "d": 2
        },
        "Power DC": {
          "v": 73.59999847,
          "u": "W",
          "d": 2
        },
        "YieldDay": {
          "v": 48,
          "u": "Wh",
          "d": 2
        },
        "YieldTotal": {
          "v": 48.5399971,
          "u": "kWh",
          "d": 2
        },
        "Frequency": {
          "v": 50.02000046,
          "u": "Hz",
          "d": 2
        },
        "Temperature": {
          "v": 11.39999962,
          "u": "°C",
          "d": 1
        },
        "PowerFactor": {
          "v": 1,
          "u": "",
          "d": 3
        },
        "ReactivePower": {
          "v": 0.100000001,
          "u": "var",
          "d": 1
        },
        "Efficiency": {
          "v": 95.24456024,
          "u": "%",
          "d": 2
        }
      },
      "1": {
        "Power": {
          "v": 36.5,
          "u": "W",
          "d": 1
        },
        "Voltage": {
          "v": 39.09999847,
          "u": "V",
          "d": 1
        },
        "Current": {
          "v": 0.930000007,
          "u": "A",
          "d": 2
        },
        "YieldDay": {
          "v": 31,
          "u": "Wh",
          "d": 0
        },
        "YieldTotal": {
          "v": 4.301000118,
          "u": "kWh",
          "d": 3
        },
        "Irradiation": {
          "v": 8.902439117,
          "u": "%",
          "d": 2
        }
      },
      "2": {
        "Power": {
          "v": 37.09999847,
          "u": "W",
          "d": 1
        },
        "Voltage": {
          "v": 40.79999924,
          "u": "V",
          "d": 1
        },
        "Current": {
          "v": 0.910000026,
          "u": "A",
          "d": 2
        },
        "YieldDay": {
          "v": 17,
          "u": "Wh",
          "d": 0
        },
        "YieldTotal": {
          "v": 44.23899841,
          "u": "kWh",
          "d": 3
        },
        "Irradiation": {
          "v": 9.048780441,
          "u": "%",
          "d": 2
        }
      },
      "events": 1
    }
  ],
  "total": {
    "Power": {
      "v": 140.7999878,
      "u": "W",
      "d": 1
    },
    "YieldDay": {
      "v": 71,
      "u": "Wh",
      "d": 0
    },
    "YieldTotal": {
      "v": 199.0449982,
      "u": "kWh",
      "d": 2
    }
  }
}
```

The eventlog can be fetched with the inverter serial number as parameter:

```
martin@bln9716cm ~/swbuild/OpenDTU $ curl --no-progress-meter http://192.168.10.10/api/eventlog/status?inv=11418186xxxx | jq
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
```
~$ curl --no-progress-meter http://192.168.10.10/api/livedata/status | jq '.total | .Power.v'
140.7999878
```

#### Get information where login is required

When config data is requested, username and password have to be provided to `curl`
Username is always `admin`, the default password is `openDTU42`. The password is used for both the admin login and the Admin-mode Access Point.

```
~$ curl --u admin:openDTU42 http://192.168.10.10/api/ntp/config
{"ntp_server":"pool.ntp.org","ntp_timezone":"CET-1CEST,M3.5.0,M10.5.0/3","ntp_timezone_descr":"Europe/Berlin"}
```

### Post information

With HTTP POST commands information can be written to the OpenDTU.

The Web API is designed to allow the web frontend in the web browser to communicate with the OpenDTU software running on the ESP32. It is not designed to be intuitive or user-friendly, so please follow the instructions here.

#### Example 1: change ntp settings

If you want to configure the ntp server setting, first fetch the information from the web API:

```
~$ curl -u "admin:password" http://192.168.10.10/api/ntp/config
{"ntp_server":"pool.ntp.org","ntp_timezone":"CET-1CEST,M3.5.0,M10.5.0/3","ntp_timezone_descr":"Europe/Berlin"}
```

Then, second step, send your new settings. Use the text output from curl in the first step, add `data=` and enclose the whole data with single quotes.

```
~$ curl -u "admin:password" http://192.168.10.10/api/ntp/config -d 'data={"ntp_server":"my.own.ntp.server.home","ntp_timezone":"CET-1CEST,M3.5.0,M10.5.0/3","ntp_timezone_descr":"Europe/Berlin"}'
{"type":"success","message":"Settings saved!"}
```
You will receive a json formatted response.

#### Example 2: change power limit

In the second example, I want to change the non persistent power limit of an inverter. Again, first fetch current data:

```
~$ curl http://192.168.10.10/api/limit/status
{"11418186xxxx":{"limit_relative":100,"max_power":600,"limit_set_status":"Ok"},"11418180xxxx":{"limit_relative":100,"max_power":800,"limit_set_status":"Ok"}}
```

I see data from two configured inverters.

Now I set the relative power limit of inverter with serialnumber `11418180xxxx` to 50%.

```
~$ curl -u "admin:password" http://192.168.10.10/api/limit/config -d 'data={"serial":"11418180xxxx", "limit_type":1, "limit_value":50}'
{"type":"success","message":"Settings saved!"}
```

Then I read again the limit status. In the first answer the status is `pending`, some seconds later it changed to `OK`.

```
~$ curl http://192.168.10.10/api/limit/status
{"11418186xxxx":{"limit_relative":100,"max_power":600,"limit_set_status":"Ok"},"11418180xxxx":{"limit_relative":100,"max_power":800,"limit_set_status":"Pending"}}

...

~$ curl http://192.168.10.10/api/limit/status
{"11418186xxxx":{"limit_relative":100,"max_power":600,"limit_set_status":"Ok"},"11418180xxxx":{"limit_relative":50,"max_power":800,"limit_set_status":"Ok"}}
```
