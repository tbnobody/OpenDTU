// SPDX-License-Identifier: GPL-2.0-or-later
#include "Configuration.h"
#include "TibberPowerMeter.h"
#include "MessageOutput.h"
#include <WiFiClientSecure.h>
#include <base64.h>
#include <ESPmDNS.h>
#include <PowerMeter.h>

bool TibberPowerMeterClass::updateValues()
{
    auto const& config = Configuration.get();

    auto const& tibberConfig = config.PowerMeter.Tibber;

    if (!query(tibberConfig)) {
        MessageOutput.printf("[TibberPowerMeter] Getting the power of tibber failed.\r\n");
        MessageOutput.printf("%s\r\n", tibberPowerMeterError);
        return false;
    }

    return true;
}

bool TibberPowerMeterClass::query(PowerMeterTibberConfig const& config)
{
    //hostByName in WiFiGeneric fails to resolve local names. issue described in
    //https://github.com/espressif/arduino-esp32/issues/3822
    //and in depth analyzed in https://github.com/espressif/esp-idf/issues/2507#issuecomment-761836300
    //in conclusion: we cannot rely on httpClient.begin(*wifiClient, url) to resolve IP adresses.
    //have to do it manually here. Feels Hacky...
    String protocol;
    String host;
    String uri;
    String base64Authorization;
    uint16_t port;
    extractUrlComponents(config.Url, protocol, host, uri, port, base64Authorization);

    IPAddress ipaddr((uint32_t)0);
    //first check if "host" is already an IP adress
    if (!ipaddr.fromString(host))
    {
        //"host"" is not an IP address so try to resolve the IP adress
        //first try locally via mDNS, then via DNS. WiFiGeneric::hostByName() will spam the console if done the otherway around.
        const bool mdnsEnabled = Configuration.get().Mdns.Enabled;
        if (!mdnsEnabled) {
            snprintf_P(tibberPowerMeterError, sizeof(tibberPowerMeterError), PSTR("Error resolving host %s via DNS, try to enable mDNS in Network Settings"), host.c_str());
            //ensure we try resolving via DNS even if mDNS is disabled
            if(!WiFiGenericClass::hostByName(host.c_str(), ipaddr)){
                    snprintf_P(tibberPowerMeterError, sizeof(tibberPowerMeterError), PSTR("Error resolving host %s via DNS"), host.c_str());
                }
        }
        else
        {
            ipaddr = MDNS.queryHost(host);
            if (ipaddr == INADDR_NONE){
                snprintf_P(tibberPowerMeterError, sizeof(tibberPowerMeterError), PSTR("Error resolving host %s via mDNS"), host.c_str());
                //when we cannot find local server via mDNS, try resolving via DNS
                if(!WiFiGenericClass::hostByName(host.c_str(), ipaddr)){
                    snprintf_P(tibberPowerMeterError, sizeof(tibberPowerMeterError), PSTR("Error resolving host %s via DNS"), host.c_str());
                }
            }
        }
    }

    // secureWifiClient MUST be created before HTTPClient
    // see discussion: https://github.com/helgeerbe/OpenDTU-OnBattery/issues/381
    std::unique_ptr<WiFiClient> wifiClient;

    bool https = protocol == "https";
    if (https) {
      auto secureWifiClient = std::make_unique<WiFiClientSecure>();
      secureWifiClient->setInsecure();
      wifiClient = std::move(secureWifiClient);
    } else {
      wifiClient = std::make_unique<WiFiClient>();
    }

    return httpRequest(*wifiClient, ipaddr.toString(), port, uri, https, config);
}

bool TibberPowerMeterClass::httpRequest(WiFiClient &wifiClient, const String& host, uint16_t port, const String& uri, bool https, PowerMeterTibberConfig const& config)
{
    if(!httpClient.begin(wifiClient, host, port, uri, https)){
        snprintf_P(tibberPowerMeterError, sizeof(tibberPowerMeterError), PSTR("httpClient.begin() failed for %s://%s"), (https ? "https" : "http"), host.c_str());
        return false;
    }

    prepareRequest(config.Timeout);

    String authString = config.Username;
    authString += ":";
    authString += config.Password;
    String auth = "Basic ";
    auth.concat(base64::encode(authString));
    httpClient.addHeader("Authorization", auth);

    int httpCode = httpClient.GET();

    if (httpCode <= 0) {
        snprintf_P(tibberPowerMeterError, sizeof(tibberPowerMeterError), PSTR("HTTP Error %s"), httpClient.errorToString(httpCode).c_str());
        return false;
    }

    if (httpCode != HTTP_CODE_OK) {
        snprintf_P(tibberPowerMeterError, sizeof(tibberPowerMeterError), PSTR("Bad HTTP code: %d"), httpCode);
        return false;
    }

    while (httpClient.getStream().available()) {
        double readVal = 0;
        unsigned char smlCurrentChar = httpClient.getStream().read();
        sml_states_t smlCurrentState = smlState(smlCurrentChar);
        if (smlCurrentState == SML_LISTEND) {
            for (auto& handler: PowerMeter.smlHandlerList) {
                if (smlOBISCheck(handler.OBIS)) {
                    handler.Fn(readVal);
                    *handler.Arg = readVal;
                }
            }
        }
    }
    httpClient.end();

    return true;
}

//extract url component as done by httpClient::begin(String url, const char* expectedProtocol) https://github.com/espressif/arduino-esp32/blob/da6325dd7e8e152094b19fe63190907f38ef1ff0/libraries/HTTPClient/src/HTTPClient.cpp#L250
bool TibberPowerMeterClass::extractUrlComponents(String url, String& _protocol, String& _host, String& _uri, uint16_t& _port, String& _base64Authorization)
{
    // check for : (http: or https:
    int index = url.indexOf(':');
    if(index < 0) {
        snprintf_P(tibberPowerMeterError, sizeof(tibberPowerMeterError), PSTR("failed to parse protocol"));
        return false;
    }

    _protocol = url.substring(0, index);

    //initialize port to default values for http or https.
    //port will be overwritten below in case port is explicitly defined
    _port = (_protocol == "https" ? 443 : 80);

    url.remove(0, (index + 3)); // remove http:// or https://

    index = url.indexOf('/');
    if (index == -1) {
        index = url.length();
        url += '/';
    }
    String host = url.substring(0, index);
    url.remove(0, index); // remove host part

    // get Authorization
    index = host.indexOf('@');
    if(index >= 0) {
        // auth info
        String auth = host.substring(0, index);
        host.remove(0, index + 1); // remove auth part including @
        _base64Authorization = base64::encode(auth);
    }

    // get port
    index = host.indexOf(':');
    String the_host;
    if(index >= 0) {
        the_host = host.substring(0, index); // hostname
        host.remove(0, (index + 1)); // remove hostname + :
        _port = host.toInt(); // get port
    } else {
        the_host = host;
    }

    _host = the_host;
    _uri = url;
    return true;
}

void TibberPowerMeterClass::prepareRequest(uint32_t timeout) {
    httpClient.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    httpClient.setUserAgent("OpenDTU-OnBattery");
    httpClient.setConnectTimeout(timeout);
    httpClient.setTimeout(timeout);
    httpClient.addHeader("Content-Type", "application/json");
    httpClient.addHeader("Accept", "application/json");
}

TibberPowerMeterClass TibberPowerMeter;
