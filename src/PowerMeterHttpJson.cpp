// SPDX-License-Identifier: GPL-2.0-or-later
#include "Configuration.h"
#include "PowerMeterHttpJson.h"
#include "MessageOutput.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "mbedtls/sha256.h"
#include <base64.h>
#include <ESPmDNS.h>

PowerMeterHttpJson::~PowerMeterHttpJson()
{
    // the wifiClient instance must live longer than the httpClient instance,
    // as the httpClient holds a pointer to the wifiClient and uses it in its
    // destructor.
    httpClient.reset();
    wifiClient.reset();
}

void PowerMeterHttpJson::loop()
{
    auto const& config = Configuration.get();
    if ((millis() - _lastPoll) < (config.PowerMeter.Interval * 1000)) {
        return;
    }

    _lastPoll = millis();

    for (uint8_t i = 0; i < POWERMETER_MAX_PHASES; i++) {
        auto const& phaseConfig = config.PowerMeter.Http_Phase[i];

        if (!phaseConfig.Enabled) {
            _cache[i] = 0.0;
            continue;
        }

        if (i == 0 || config.PowerMeter.HttpIndividualRequests) {
            if (!queryPhase(i, phaseConfig)) {
                MessageOutput.printf("[PowerMeterHttpJson] Getting HTTP response for phase %d failed.\r\n", i + 1);
                MessageOutput.printf("%s\r\n", httpPowerMeterError);
                return;
            }
            continue;
        }

        if(!tryGetFloatValueForPhase(i, phaseConfig.JsonPath, phaseConfig.PowerUnit, phaseConfig.SignInverted)) {
            MessageOutput.printf("[PowerMeterHttpJson] Reading power of phase %d (from JSON fetched with Phase 1 config) failed.\r\n", i + 1);
            MessageOutput.printf("%s\r\n", httpPowerMeterError);
            return;
        }
    }

    gotUpdate();

    _powerValues = _cache;
}

float PowerMeterHttpJson::getPowerTotal() const
{
    float sum = 0.0;
    for (auto v: _powerValues) { sum += v; }
    return sum;
}

void PowerMeterHttpJson::doMqttPublish() const
{
    mqttPublish("power1", _powerValues[0]);
    mqttPublish("power2", _powerValues[1]);
    mqttPublish("power3", _powerValues[2]);
}

bool PowerMeterHttpJson::queryPhase(int phase, PowerMeterHttpConfig const& config)
{
    //hostByName in WiFiGeneric fails to resolve local names. issue described in
    //https://github.com/espressif/arduino-esp32/issues/3822
    //and in depth analyzed in https://github.com/espressif/esp-idf/issues/2507#issuecomment-761836300
    //in conclusion: we cannot rely on httpClient->begin(*wifiClient, url) to resolve IP adresses.
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

    bool https = protocol == "https";
    if (https) {
      auto secureWifiClient = std::make_unique<WiFiClientSecure>();
      secureWifiClient->setInsecure();
      wifiClient = std::move(secureWifiClient);
    } else {
      wifiClient = std::make_unique<WiFiClient>();
    }

    return httpRequest(phase, ipaddr.toString(), port, uri, https, config);
}

bool PowerMeterHttpJson::httpRequest(int phase, const String& host, uint16_t port, const String& uri, bool https, PowerMeterHttpConfig const& config)
{
    if (!httpClient) { httpClient = std::make_unique<HTTPClient>(); }

    if(!httpClient->begin(*wifiClient, host, port, uri, https)){
        snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("httpClient->begin() failed for %s://%s"), (https ? "https" : "http"), host.c_str());
        return false;
    }

    prepareRequest(config.Timeout, config.HeaderKey, config.HeaderValue);
    if (config.AuthType == Auth_t::Digest) {
        const char *headers[1] = {"WWW-Authenticate"};
        httpClient->collectHeaders(headers, 1);
    } else if (config.AuthType == Auth_t::Basic) {
        String authString = config.Username;
        authString += ":";
        authString += config.Password;
        String auth = "Basic ";
        auth.concat(base64::encode(authString));
        httpClient->addHeader("Authorization", auth);
    }
    int httpCode = httpClient->GET();

    if (httpCode == HTTP_CODE_UNAUTHORIZED && config.AuthType == Auth_t::Digest) {
        // Handle authentication challenge
        if (httpClient->hasHeader("WWW-Authenticate")) {
            String authReq  = httpClient->header("WWW-Authenticate");
            String authorization = getDigestAuth(authReq, String(config.Username), String(config.Password), "GET", String(uri), 1);
            httpClient->end();
            if(!httpClient->begin(*wifiClient, host, port, uri, https)){
                snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("httpClient->begin() failed for  %s://%s using digest auth"), (https ? "https" : "http"), host.c_str());
                return false;
            }

            prepareRequest(config.Timeout, config.HeaderKey, config.HeaderValue);
            httpClient->addHeader("Authorization", authorization);
            httpCode = httpClient->GET();
        }
    }

    if (httpCode <= 0) {
        snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("HTTP Error %s"), httpClient->errorToString(httpCode).c_str());
        return false;
    }

    if (httpCode != HTTP_CODE_OK) {
        snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("Bad HTTP code: %d"), httpCode);
        return false;
    }

    httpResponse = httpClient->getString(); // very unfortunate that we cannot parse WifiClient stream directly
    httpClient->end();

    // TODO(schlimmchen): postpone calling tryGetFloatValueForPhase, as it
    // will be called twice for each phase when doing separate requests.
    return tryGetFloatValueForPhase(phase, config.JsonPath, config.PowerUnit, config.SignInverted);
}

String PowerMeterHttpJson::extractParam(String& authReq, const String& param, const char delimit) {
    int _begin = authReq.indexOf(param);
    if (_begin == -1) { return ""; }
    return authReq.substring(_begin + param.length(), authReq.indexOf(delimit, _begin + param.length()));
}

String PowerMeterHttpJson::getcNonce(const int len) {
    static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz";
    String s = "";

    for (int i = 0; i < len; ++i) { s += alphanum[rand() % (sizeof(alphanum) - 1)]; }

    return s;
}

String PowerMeterHttpJson::getDigestAuth(String& authReq, const String& username, const String& password, const String& method, const String& uri, unsigned int counter) {
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

bool PowerMeterHttpJson::tryGetFloatValueForPhase(int phase, String jsonPath, Unit_t unit, bool signInverted)
{
    JsonDocument root;
    const DeserializationError error = deserializeJson(root, httpResponse);
    if (error) {
        snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError),
                PSTR("[PowerMeterHttpJson] Unable to parse server response as JSON"));
        return false;
    }

    constexpr char delimiter = '/';
    int start = 0;
    int end = jsonPath.indexOf(delimiter);
    auto value = root.as<JsonVariantConst>();

    auto getNext = [this, &value, &jsonPath, &start](String const& key) -> bool {
        // handle double forward slashes and paths starting or ending with a slash
        if (key.isEmpty()) { return true; }

        if (key[0] == '[' && key[key.length() - 1] == ']') {
            if (!value.is<JsonArrayConst>()) {
                snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError),
                        PSTR("[HttpPowerMeter] Cannot access non-array JSON node "
                            "using array index '%s' (JSON path '%s', position %i)"),
                        key.c_str(), jsonPath.c_str(), start);
                return false;
            }

            auto idx = key.substring(1, key.length() - 1).toInt();
            value = value[idx];

            if (value.isNull()) {
                snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError),
                        PSTR("[HttpPowerMeter] Unable to access JSON array "
                            "index %li (JSON path '%s', position %i)"),
                        idx, jsonPath.c_str(), start);
                return false;
            }

            return true;
        }

        value = value[key];

        if (value.isNull()) {
            snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError),
                    PSTR("[HttpPowerMeter] Unable to access JSON key "
                        "'%s' (JSON path '%s', position %i)"),
                    key.c_str(), jsonPath.c_str(), start);
            return false;
        }

        return true;
    };

    // NOTE: "Because ArduinoJson implements the Null Object Pattern, it is
    // always safe to read the object: if the key doesn't exist, it returns an
    // empty value."
    while (end != -1) {
        if (!getNext(jsonPath.substring(start, end))) { return false; }
        start = end + 1;
        end = jsonPath.indexOf(delimiter, start);
    }

    if (!getNext(jsonPath.substring(start))) { return false; }

    if (!value.is<float>()) {
        snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError),
                PSTR("[PowerMeterHttpJson] not a float: '%s'"),
                value.as<String>().c_str());
        return false;
    }

    // this value is supposed to be in Watts and positive if energy is consumed.
    _cache[phase] = value.as<float>();

    switch (unit) {
        case Unit_t::MilliWatts:
            _cache[phase] /= 1000;
            break;
        case Unit_t::KiloWatts:
            _cache[phase] *= 1000;
            break;
        default:
            break;
    }

    if (signInverted) { _cache[phase] *= -1; }

    return true;
}

//extract url component as done by httpClient::begin(String url, const char* expectedProtocol) https://github.com/espressif/arduino-esp32/blob/da6325dd7e8e152094b19fe63190907f38ef1ff0/libraries/HTTPClient/src/HTTPClient.cpp#L250
bool PowerMeterHttpJson::extractUrlComponents(String url, String& _protocol, String& _host, String& _uri, uint16_t& _port, String& _base64Authorization)
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

String PowerMeterHttpJson::sha256(const String& data) {
    uint8_t hash[32];

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // select SHA256
    mbedtls_sha256_update(&ctx, reinterpret_cast<const unsigned char*>(data.c_str()), data.length());
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);

    char res[sizeof(hash) * 2 + 1];
    for (int i = 0; i < sizeof(hash); i++) {
        snprintf(res + (i*2), sizeof(res) - (i*2), "%02x", hash[i]);
    }

    return res;
}

void PowerMeterHttpJson::prepareRequest(uint32_t timeout, const char* httpHeader, const char* httpValue) {
    httpClient->setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    httpClient->setUserAgent("OpenDTU-OnBattery");
    httpClient->setConnectTimeout(timeout);
    httpClient->setTimeout(timeout);
    httpClient->addHeader("Content-Type", "application/json");
    httpClient->addHeader("Accept", "application/json");

    if (strlen(httpHeader) > 0) {
        httpClient->addHeader(httpHeader, httpValue);
    }
}
