// SPDX-License-Identifier: GPL-2.0-or-later
#include "Configuration.h"
#include "HttpPowerMeter.h"
#include "MessageOutput.h"
#include <WiFiClientSecure.h>
#include <FirebaseJson.h>
#include <Crypto.h>
#include <SHA256.h>
#include <memory>

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

    char response[2000],
        errorMessage[256];

    for (uint8_t i = 0; i < POWERMETER_MAX_PHASES; i++) {
        POWERMETER_HTTP_PHASE_CONFIG_T phaseConfig = config.Powermeter_Http_Phase[i];

        if (!phaseConfig.Enabled) {
            power[i] = 0.0;
            continue;	
        } 

        if (i == 0 || config.PowerMeter_HttpIndividualRequests) {
            if (httpRequest(phaseConfig.Url, phaseConfig.AuthType, phaseConfig.Username, phaseConfig.Password, phaseConfig.HeaderKey, phaseConfig.HeaderValue, phaseConfig.Timeout,
                response, sizeof(response), errorMessage, sizeof(errorMessage))) {
                  if (!getFloatValueByJsonPath(response, phaseConfig.JsonPath, power[i])) {
                      MessageOutput.printf("[HttpPowerMeter] Couldn't find a value with Json query \"%s\"\r\n", phaseConfig.JsonPath);
                      return false;
                  }
            } else {
                MessageOutput.printf("[HttpPowerMeter] Getting the power of phase %d failed. Error: %s\r\n",
                    i + 1, errorMessage);
                return false;
            }
        }
    }

    return true;
}

bool HttpPowerMeterClass::httpRequest(const char* url, Auth authType, const char* username, const char* password, const char* httpHeader, const char* httpValue, uint32_t timeout,
        char* response, size_t responseSize, char* error, size_t errorSize)
{

    String newUrl = url;
    String urlProtocol;
    String urlHostname;
    String urlUri;
    extractUrlComponents(url, urlProtocol, urlHostname, urlUri);

    response[0] = '\0';
    error[0] = '\0';

    if (authType == Auth::basic) {
        newUrl = urlProtocol;
        newUrl += "://";
        newUrl += username;
        newUrl += ":";
        newUrl += password;
        newUrl += "@";
        newUrl += urlHostname;
        newUrl += urlUri;
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

   
    if (!httpClient.begin(*wifiClient, newUrl)) {
      snprintf_P(error, errorSize, "httpClient.begin(%s) failed", newUrl.c_str());
      return false;
    }
    prepareRequest(timeout, httpHeader, httpValue);
    
    if (authType == Auth::digest) {
        const char *headers[1] = {"WWW-Authenticate"};
        httpClient.collectHeaders(headers, 1);
    }

    int httpCode = httpClient.GET();
    if (httpCode == HTTP_CODE_UNAUTHORIZED && authType == Auth::digest) {
        // Handle authentication challenge
        char realm[256];  // Buffer to store the realm received from the server
        char nonce[256];  // Buffer to store the nonce received from the server
        if (httpClient.hasHeader("WWW-Authenticate")) {
            String authHeader = httpClient.header("WWW-Authenticate");
            if (authHeader.indexOf("Digest") != -1) {
                int realmIndex = authHeader.indexOf("realm=\"");
                int nonceIndex = authHeader.indexOf("nonce=\"");
                if (realmIndex != -1 && nonceIndex != -1) {
                    int realmEndIndex = authHeader.indexOf("\"", realmIndex + 7);
                    int nonceEndIndex = authHeader.indexOf("\"", nonceIndex + 7);
                    if (realmEndIndex != -1 && nonceEndIndex != -1) {
                        authHeader.substring(realmIndex + 7, realmEndIndex).toCharArray(realm, sizeof(realm));
                        authHeader.substring(nonceIndex + 7, nonceEndIndex).toCharArray(nonce, sizeof(nonce));
                    }
                }
                String cnonce = String(random(1000)); // Generate client nonce
                String str = username;
                str += ":";
                str += realm;
                str += ":";
                str += password;
                String ha1 = sha256(str);
                str = "GET:";
                str += urlUri;
                String ha2 = sha256(str);
                str = ha1;
                str += ":";
                str += nonce;
                str +=  ":00000001:";
                str += cnonce;
                str += ":auth:";
                str +=  ha2;
                String response = sha256(str);

                String authorization = "Digest username=\""; 
                authorization += username;
                authorization += "\", realm=\"";
                authorization += realm;
                authorization += "\", nonce=\"";
                authorization += nonce;
                authorization += "\", uri=\"";
                authorization += urlUri;
                authorization += "\", cnonce=\"";
                authorization += cnonce;
                authorization += "\", nc=00000001, qop=auth, response=\"";
                authorization += response;
                authorization += "\", algorithm=SHA-256";
                httpClient.end();
                if (!httpClient.begin(*wifiClient, newUrl)) {
                    snprintf_P(error, errorSize, "httpClient.begin(%s) for digest auth failed", newUrl.c_str());
                    return false;
                }
                prepareRequest(timeout, httpHeader, httpValue);
                httpClient.addHeader("Authorization", authorization);
                httpCode = httpClient.GET();
            }
        }        
    }

    if (httpCode == HTTP_CODE_OK) {
        String responseBody = httpClient.getString();

        if (responseBody.length() > (responseSize - 1)) {
            snprintf_P(error, errorSize, "Response too large! Response length: %d Body start: %s",
                httpClient.getSize(), responseBody.c_str());
        } else {
            snprintf(response, responseSize, responseBody.c_str());
        }
    } else if (httpCode <= 0) {
        snprintf_P(error, errorSize, "Error(%s): %s", newUrl.c_str(), httpClient.errorToString(httpCode).c_str());
    } else if (httpCode != HTTP_CODE_OK) {
        snprintf_P(error, errorSize, "Bad HTTP code: %d", httpCode);
    }

    httpClient.end();

    if (error[0] != '\0') {
        return false;
    }

    return true;
}

float HttpPowerMeterClass::getFloatValueByJsonPath(const char* jsonString, const char* jsonPath, float& value)
{
    FirebaseJson firebaseJson;
    firebaseJson.setJsonData(jsonString);

    FirebaseJsonData firebaseJsonResult;
    if (!firebaseJson.get(firebaseJsonResult, jsonPath)) {
        return false;
    }

    value = firebaseJsonResult.to<float>();

    firebaseJson.clear();

    return true;
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
