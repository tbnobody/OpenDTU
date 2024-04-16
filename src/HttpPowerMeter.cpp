// SPDX-License-Identifier: GPL-2.0-or-later
#include "Configuration.h"
#include "HttpPowerMeter.h"
#include "MessageOutput.h"
#include <WiFiClientSecure.h>
#include <FirebaseJson.h>
#include <Crypto.h>
#include <SHA256.h>
#include <base64.h>
#include <memory>
#include <ESPmDNS.h>

void HttpPowerMeterClass::init()
{
}

float HttpPowerMeterClass::getPower(int8_t phase)
{
    return power[phase - 1];
}

bool HttpPowerMeterClass::updateValues()
{
    auto const& config = Configuration.get();

    for (uint8_t i = 0; i < POWERMETER_MAX_PHASES; i++) {
        auto const& phaseConfig = config.PowerMeter.Http_Phase[i];

        if (!phaseConfig.Enabled) {
            power[i] = 0.0;
            continue;
        }

        if (i == 0 || config.PowerMeter.HttpIndividualRequests) {
            if (!queryPhase(i, phaseConfig)) {
                MessageOutput.printf("[HttpPowerMeter] Getting the power of phase %d failed.\r\n", i + 1);
                MessageOutput.printf("%s\r\n", httpPowerMeterError);
                return false;
            }
            continue;
        }

        if(!tryGetFloatValueForPhase(i, phaseConfig.JsonPath)) {
            MessageOutput.printf("[HttpPowerMeter] Getting the power of phase %d (from JSON fetched with Phase 1 config) failed.\r\n", i + 1);
            MessageOutput.printf("%s\r\n", httpPowerMeterError);
            return false;
        }
    }
    return true;
}

bool HttpPowerMeterClass::queryPhase(int phase, PowerMeterHttpConfig const& config)
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
            snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("Error resolving host %s via DNS, try to enable mDNS in Network Settings"), host.c_str());
            //ensure we try resolving via DNS even if mDNS is disabled
            if(!WiFiGenericClass::hostByName(host.c_str(), ipaddr)){
                    snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("Error resolving host %s via DNS"), host.c_str());
                }
        }
        else
        {
            ipaddr = MDNS.queryHost(host);
            if (ipaddr == INADDR_NONE){
                snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("Error resolving host %s via mDNS"), host.c_str());
                //when we cannot find local server via mDNS, try resolving via DNS
                if(!WiFiGenericClass::hostByName(host.c_str(), ipaddr)){
                    snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("Error resolving host %s via DNS"), host.c_str());
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

    return httpRequest(phase, *wifiClient, ipaddr.toString(), port, uri, https, config);
}

bool HttpPowerMeterClass::httpRequest(int phase, WiFiClient &wifiClient, const String& host, uint16_t port, const String& uri, bool https, PowerMeterHttpConfig const& config)
{
    if(!httpClient.begin(wifiClient, host, port, uri, https)){
        snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("httpClient.begin() failed for %s://%s"), (https ? "https" : "http"), host.c_str());
        return false;
    }

    prepareRequest(config.Timeout, config.HeaderKey, config.HeaderValue);
    if (config.AuthType == Auth_t::Digest) {
        const char *headers[1] = {"WWW-Authenticate"};
        httpClient.collectHeaders(headers, 1);
    } else if (config.AuthType == Auth_t::Basic) {
        String authString = config.Username;
        authString += ":";
        authString += config.Password;
        String auth = "Basic ";
        auth.concat(base64::encode(authString));
        httpClient.addHeader("Authorization", auth);
    }
    int httpCode = httpClient.GET();

    if (httpCode == HTTP_CODE_UNAUTHORIZED && config.AuthType == Auth_t::Digest) {
        // Handle authentication challenge
        if (httpClient.hasHeader("WWW-Authenticate")) {
            String authReq  = httpClient.header("WWW-Authenticate");
            String authorization = getDigestAuth(authReq, String(config.Username), String(config.Password), "GET", String(uri), 1);
            httpClient.end();
            if(!httpClient.begin(wifiClient, host, port, uri, https)){
                snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("httpClient.begin() failed for  %s://%s using digest auth"), (https ? "https" : "http"), host.c_str());
                return false;
            }

            prepareRequest(config.Timeout, config.HeaderKey, config.HeaderValue);
            httpClient.addHeader("Authorization", authorization);
            httpCode = httpClient.GET();
        }
    }

    if (httpCode <= 0) {
        snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("HTTP Error %s"), httpClient.errorToString(httpCode).c_str());
        return false;
    }

    if (httpCode != HTTP_CODE_OK) {
        snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("Bad HTTP code: %d"), httpCode);
        return false;
    }

    httpResponse = httpClient.getString(); // very unfortunate that we cannot parse WifiClient stream directly
    httpClient.end();

    return tryGetFloatValueForPhase(phase, jsonPath);
}

String HttpPowerMeterClass::extractParam(String& authReq, const String& param, const char delimit) {
    int _begin = authReq.indexOf(param);
    if (_begin == -1) { return ""; }
    return authReq.substring(_begin + param.length(), authReq.indexOf(delimit, _begin + param.length()));
}

String HttpPowerMeterClass::getcNonce(const int len) {
    static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz";
    String s = "";

    for (int i = 0; i < len; ++i) { s += alphanum[rand() % (sizeof(alphanum) - 1)]; }

    return s;
}

String HttpPowerMeterClass::getDigestAuth(String& authReq, const String& username, const String& password, const String& method, const String& uri, unsigned int counter) {
    // extracting required parameters for RFC 2617 Digest
    String realm = extractParam(authReq, "realm=\"", '"');
    String nonce = extractParam(authReq, "nonce=\"", '"');
    String cNonce = getcNonce(8);

    char nc[9];
    snprintf(nc, sizeof(nc), "%08x", counter);

    //sha256 of the user:realm:password
    String ha1 = sha256(username + ":" + realm + ":" + password);

    //sha256 of method:uri
    String ha2 = sha256(method + ":" + uri);

    //sha256 of h1:nonce:nc:cNonce:auth:h2
    String response = sha256(ha1 + ":" + nonce + ":" + String(nc) + ":" + cNonce + ":" + "auth" + ":" + ha2);

    //Final authorization String;
    String authorization = "Digest username=\"";
    authorization += username;
    authorization += "\", realm=\"";
    authorization += realm;
    authorization += "\", nonce=\"";
    authorization += nonce;
    authorization += "\", uri=\"";
    authorization += uri;
    authorization += "\", cnonce=\"";
    authorization += cNonce;
    authorization += "\", nc=";
    authorization += String(nc);
    authorization += ", qop=auth, response=\"";
    authorization += response;
    authorization += "\", algorithm=SHA-256";

    return authorization;
}

bool HttpPowerMeterClass::tryGetFloatValueForPhase(int phase, const char* jsonPath)
{
    FirebaseJson json;
    json.setJsonData(httpResponse);
    FirebaseJsonData value;
    if (!json.get(value, jsonPath)) {
        snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("[HttpPowerMeter] Couldn't find a value for phase %i with Json query \"%s\""), phase, jsonPath);
        return false;
    }

    power[phase] = value.to<float>();
    return true;
}

//extract url component as done by httpClient::begin(String url, const char* expectedProtocol) https://github.com/espressif/arduino-esp32/blob/da6325dd7e8e152094b19fe63190907f38ef1ff0/libraries/HTTPClient/src/HTTPClient.cpp#L250
bool HttpPowerMeterClass::extractUrlComponents(String url, String& _protocol, String& _host, String& _uri, uint16_t& _port, String& _base64Authorization)
{
    // check for : (http: or https:
    int index = url.indexOf(':');
    if(index < 0) {
        snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("failed to parse protocol"));
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

#define HASH_SIZE 32

String HttpPowerMeterClass::sha256(const String& data) {
    SHA256 sha256;
    uint8_t hash[HASH_SIZE];

    sha256.reset();
    sha256.update(data.c_str(), data.length());
    sha256.finalize(hash, HASH_SIZE);

    String hashStr = "";
    for (int i = 0; i < HASH_SIZE; i++) {
        String hex = String(hash[i], HEX);
        if (hex.length() == 1) {
        hashStr += "0";
        }
        hashStr += hex;
    }

    return hashStr;
}
void HttpPowerMeterClass::prepareRequest(uint32_t timeout, const char* httpHeader, const char* httpValue) {
    httpClient.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    httpClient.setUserAgent("OpenDTU-OnBattery");
    httpClient.setConnectTimeout(timeout);
    httpClient.setTimeout(timeout);
    httpClient.addHeader("Content-Type", "application/json");
    httpClient.addHeader("Accept", "application/json");

    if (strlen(httpHeader) > 0) {
        httpClient.addHeader(httpHeader, httpValue);
    }
}

HttpPowerMeterClass HttpPowerMeter;
