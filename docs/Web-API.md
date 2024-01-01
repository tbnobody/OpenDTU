# Web API

This documentation will has been moved and can be found here: <https://tbnobody.github.io/OpenDTU-docs/firmware/web_api/>

## List of URLs

This list may be incomplete

| GET/POST | Auth required | URL |
| -------- | --- | -- |
| Get      | no  | /api/vedirectlivedata/status |
| Get      | no  | /api/vedirect/status |
| Get      | no  | /api/huawei/status |
| Get      | no  | /api/huawei/config |
| Get      | no  | /api/huawei/limit/config |
| Get      | no  | /api/batterylivedata/status |
| Get      | no  | /api/battery/status |
| Get      | no  | /api/powerlimiter/status |

### Victron REST-API (/api/vedirectlivedata/status):
````JSON
{
"data_age":0,
"age_critical":false,
"PID":"SmartSolar MPPT 100|30",
"SER":"XXX",
"FW":"159",
"LOAD":"ON",
"CS":"Bulk",
"ERR":"No error",
"OR":"Not off",
"MPPT":"MPP Tracker active",
"HSDS":{"v":46,"u":"Days"},
"V":{"v":26.36,"u":"V"},
"I":{"v":3.4,"u":"A"},
"VPV":{"v":37.13,"u":"V"},
"PPV":{"v":93,"u":"W"},
"H19":{"v":83.16,"u":"kWh"},
"H20":{"v":1.39,"u":"kWh"},
"H21":{"v":719,"u":"W"},
"H22":{"v":1.43,"u":"kWh"},
"H23":{"v":737,"u":"W"}
}
````