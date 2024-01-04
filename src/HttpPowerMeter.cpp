// SPDX-License-Identifier: GPL-2.0-or-later
#include "Configuration.h"
#include "HttpPowerMeter.h"
#include "MessageOutput.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>//saves 20kB to not use FirebaseJson as ArduinoJson is used already elsewhere (e.g. in WebApi_powermeter)
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
    const CONFIG_T& config = Configuration.get();     

    for (uint8_t i = 0; i < POWERMETER_MAX_PHASES; i++) {
        POWERMETER_HTTP_PHASE_CONFIG_T phaseConfig = config.PowerMeter.Http_Phase[i];
        String urlProtocol;
        String urlHostname;
        String urlUri;
        extractUrlComponents(phaseConfig.Url, urlProtocol, urlHostname, urlUri);

        if (!phaseConfig.Enabled) {
            power[i] = 0.0;
            continue;	
        } 

        if (i == 0 || config.PowerMeter.HttpIndividualRequests) {
            if (!queryPhase(i, urlProtocol, urlHostname, urlUri, phaseConfig.AuthType, phaseConfig.Username, phaseConfig.Password, phaseConfig.HeaderKey, phaseConfig.HeaderValue, phaseConfig.Timeout, 
                    phaseConfig.JsonPath)) {
                MessageOutput.printf("[HttpPowerMeter] Getting the power of phase %d failed.\r\n", i + 1);
                MessageOutput.printf("%s\r\n", httpPowerMeterError);
                return false;
            }
        }
    }
    return true;
}

bool HttpPowerMeterClass::queryPhase(int phase, const String& urlProtocol, const String& urlHostname, const String& uri, Auth authType, const char* username, const char* password, 
    const char* httpHeader, const char* httpValue, uint32_t timeout, const char* jsonPath)
{
    //hostByName in WiFiGeneric fails to resolve local names. issue described in 
    //https://github.com/espressif/arduino-esp32/issues/3822
    //and in depth analyzed in https://github.com/espressif/esp-idf/issues/2507#issuecomment-761836300
    //in conclusion: we cannot rely on httpClient.begin(*wifiClient, url) to resolve IP adresses.
    //have to do it manually here. Feels Hacky...
    IPAddress ipaddr((uint32_t)0);
    //first check if the urlHostname is already an IP adress    
    if (!ipaddr.fromString(urlHostname))
    {
        //no it is not, so try to resolve the IP adress
        const bool mdnsEnabled = Configuration.get().Mdns.Enabled;
        if (!mdnsEnabled) {
            snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("Enable mDNS in Network Settings")); 
            return false;
        }

        ipaddr = MDNS.queryHost(urlHostname); 
        if (ipaddr == INADDR_NONE){
            snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("Error resolving url %s"), urlHostname.c_str()); 
            return false;
        }
    }

    // secureWifiClient MUST be created before HTTPClient
    // see discussion: https://github.com/helgeerbe/OpenDTU-OnBattery/issues/381
    std::unique_ptr<WiFiClient> wifiClient;

    if (urlProtocol == "https") {
      auto secureWifiClient = std::make_unique<WiFiClientSecure>();
      secureWifiClient->setInsecure();
      wifiClient = std::move(secureWifiClient);
    } else {
      wifiClient = std::make_unique<WiFiClient>();
    }
    return httpRequest(phase, *wifiClient, urlProtocol, ipaddr.toString(), uri, authType,  username, password, httpHeader, httpValue, timeout, jsonPath);
}
bool HttpPowerMeterClass::httpRequest(int phase, WiFiClient &wifiClient, const String& urlProtocol, const String& urlHostname, const String& uri, Auth authType, const char* username,
    const char* password, const char* httpHeader, const char* httpValue, uint32_t timeout, const char* jsonPath)
{
    int port = 80;
    if (urlProtocol == "https") {
        port = 443;
    }
    if(!httpClient.begin(wifiClient, urlHostname, port, uri)){      
        snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("httpClient.begin() failed for %s://%s"), urlProtocol.c_str(), urlHostname.c_str()); 
        return false;
    }

    prepareRequest(timeout, httpHeader, httpValue);    
    if (authType == Auth::digest) {
        const char *headers[1] = {"WWW-Authenticate"};
        httpClient.collectHeaders(headers, 1);
    } else if (authType == Auth::basic) {
        String authString = username;
        authString += ":";
        authString += password;
        String auth = "Basic ";
        auth.concat(base64::encode(authString));
        httpClient.addHeader("Authorization", auth);
    }
    int httpCode = httpClient.GET();

    if (httpCode == HTTP_CODE_UNAUTHORIZED && authType == Auth::digest) {
        // Handle authentication challenge
        if (httpClient.hasHeader("WWW-Authenticate")) {
            String authReq  = httpClient.header("WWW-Authenticate");
            String authorization = getDigestAuth(authReq, String(username), String(password), "GET", String(uri), 1);
            httpClient.end();
            if(!httpClient.begin(wifiClient, urlHostname, port, uri)){     
                snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("httpClient.begin() failed for  %s://%s using digest auth"), urlProtocol.c_str(), urlHostname.c_str()); 
                return false;
            }

            prepareRequest(timeout, httpHeader, httpValue);
            httpClient.addHeader("Authorization", authorization);
            httpCode = httpClient.GET();
        }
    }
    bool result = tryGetFloatValueForPhase(phase, httpCode, jsonPath);
    httpClient.end();
    return result;
}

String HttpPowerMeterClass::extractParam(String& authReq, const String& param, const char delimit) {
  int _begin = authReq.indexOf(param);
  if (_begin == -1) { return ""; }
  return authReq.substring(_begin + param.length(), authReq.indexOf(delimit, _begin + param.length()));
}
void HttpPowerMeterClass::getcNonce(char* cNounce) {
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz";
  auto len=sizeof(cNounce);

  for (int i = 0; i < len; ++i) { cNounce[i] = alphanum[rand() % (sizeof(alphanum) - 1)]; }

}
String HttpPowerMeterClass::getDigestAuth(String& authReq, const String& username, const String& password, const String& method, const String& uri, unsigned int counter) {
  // extracting required parameters for RFC 2069 simpler Digest
  String realm = extractParam(authReq, "realm=\"", '"');
  String nonce = extractParam(authReq, "nonce=\"", '"');
  char cNonce[8];
  getcNonce(cNonce);

  char nc[9];
  snprintf(nc, sizeof(nc), "%08x", counter);

  // parameters for the Digest 
  // sha256 of the user:realm:user
  char h1Prep[sizeof(username)+sizeof(realm)+sizeof(password)+2];
  snprintf(h1Prep, sizeof(h1Prep), "%s:%s:%s", username.c_str(),realm.c_str(), password.c_str());
  String ha1 = sha256(h1Prep);

  //sha256 of method:uri
  char h2Prep[sizeof(method) + sizeof(uri) + 1];
  snprintf(h2Prep, sizeof(h2Prep), "%s:%s", method.c_str(),uri.c_str());
  String ha2 = sha256(h2Prep);

  //md5 of h1:nonce:nc:cNonce:auth:h2
  char responsePrep[sizeof(ha1)+sizeof(nc)+sizeof(cNonce)+4+sizeof(ha2) + 5];
  snprintf(responsePrep, sizeof(responsePrep), "%s:%s:%s:%s:auth:%s", ha1.c_str(),nonce.c_str(), nc, cNonce,ha2.c_str());
  String response = sha256(responsePrep);

  //Final authorization String;
  char authorization[17 + sizeof(username) + 10 + sizeof(realm) + 10 + sizeof(nonce) + 8 + sizeof(uri) + 34 + sizeof(nc) + 10 + sizeof(cNonce) + 13 + sizeof(response)];
  snprintf(authorization, sizeof(authorization), "Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", algorithm=SHA-256, qop=auth, nc=%s, cnonce=\"%s\", response=\"%s\"", username.c_str(), realm.c_str(), nonce.c_str(), uri.c_str(), nc, cNonce, response.c_str());

  return authorization;
}
bool HttpPowerMeterClass::tryGetFloatValueForPhase(int phase, int httpCode, const char* jsonPath)
{
    bool success = false;
    if (httpCode == HTTP_CODE_OK) {
        httpResponse = httpClient.getString();     //very unfortunate that we cannot parse WifiClient stream directly   
        StaticJsonDocument<2048> json;             //however creating these allocations on stack should be fine to avoid heap fragmentation
        deserializeJson(json, httpResponse);
        if(!json.containsKey(jsonPath))
        {
            snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("[HttpPowerMeter] Couldn't find a value for phase %i with Json query \"%s\""), phase, jsonPath);
        }else {
            power[phase] = json[jsonPath].as<float>();
            //MessageOutput.printf("Power for Phase %i: %5.2fW\r\n", phase, power[phase]);
            success = true;
        }
    } else if (httpCode <= 0) {
        snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("HTTP Error %s"), httpClient.errorToString(httpCode).c_str());
    } else if (httpCode != HTTP_CODE_OK) {
        snprintf_P(httpPowerMeterError, sizeof(httpPowerMeterError), PSTR("Bad HTTP code: %d"), httpCode);
    } 
    return success;
}
void HttpPowerMeterClass::extractUrlComponents(const String& url, String& protocol, String& hostname, String& uri) {
    // Find protocol delimiter
    int protocolEndIndex = url.indexOf(":");
    if (protocolEndIndex != -1) {
        protocol = url.substring(0, protocolEndIndex);

        // Find double slash delimiter
        int doubleSlashIndex = url.indexOf("//", protocolEndIndex);
        if (doubleSlashIndex != -1) {
            // Find slash after double slash delimiter
            int slashIndex = url.indexOf("/", doubleSlashIndex + 2);
            if (slashIndex != -1) {
                // Extract hostname and uri
                hostname = url.substring(doubleSlashIndex + 2, slashIndex);
                uri = url.substring(slashIndex);
            } else {
                // No slash after double slash delimiter, so the whole remaining part is the hostname
                hostname = url.substring(doubleSlashIndex + 2);
                uri = "/";
            }
        }
    }

    // Remove username:password if present in the hostname
    int atIndex = hostname.indexOf("@");
    if (atIndex != -1) {
        hostname = hostname.substring(atIndex + 1);
    }
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
