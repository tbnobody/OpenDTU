// SPDX-License-Identifier: GPL-2.0-or-later
#include "Configuration.h"
#include "HttpPowerMeter.h"
#include "MessageOutput.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <FirebaseJson.h>

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
    bool success = true;

    for (uint8_t i = 0; i < POWERMETER_MAX_PHASES; i++) {
        POWERMETER_HTTP_PHASE_CONFIG_T phaseConfig = config.Powermeter_Http_Phase[i];

        if (!phaseConfig.Enabled || !success) {
            power[i] = 0.0;
            continue;
        }

        if (i == 0 || config.PowerMeter_HttpIndividualRequests) {
            if (!httpRequest(phaseConfig.Url, phaseConfig.HeaderKey, phaseConfig.HeaderValue, phaseConfig.Timeout,
                response, sizeof(response), errorMessage, sizeof(errorMessage))) {
                MessageOutput.printf("[HttpPowerMeter] Getting the power of phase %d failed. Error: %s\r\n",
                    i + 1, errorMessage);
                success = false;
            }
        }

        if (!getFloatValueByJsonPath(response, phaseConfig.JsonPath, power[i])) {
            MessageOutput.printf("[HttpPowerMeter] Couldn't find a value with Json query \"%s\"\r\n", phaseConfig.JsonPath);
            success = false;
        }
    }

    return success;
}

bool HttpPowerMeterClass::httpRequest(const char* url, const char* httpHeader, const char* httpValue, uint32_t timeout,
        char* response, size_t responseSize, char* error, size_t errorSize)
{
    WiFiClient* wifiClient = NULL;
    HTTPClient httpClient;

    response[0] = '\0';
    error[0] = '\0';

    if (String(url).substring(0, 6) == "https:") {
        wifiClient = new WiFiClientSecure;
        reinterpret_cast<WiFiClientSecure*>(wifiClient)->setInsecure();
    } else {
        wifiClient = new WiFiClient;
    }

    if (!httpClient.begin(*wifiClient, url)) {
        snprintf_P(error, errorSize, "httpClient.begin failed");
        delete wifiClient;
        return false;
    }

    httpClient.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    httpClient.setUserAgent("OpenDTU-OnBattery");
    httpClient.setConnectTimeout(timeout);
    httpClient.setTimeout(timeout);
    httpClient.addHeader("Content-Type", "application/json");
    httpClient.addHeader("Accept", "application/json");

    if (strlen(httpHeader) > 0) {
        httpClient.addHeader(httpHeader, httpValue);
    }

    int httpCode = httpClient.GET();


    if (httpCode == HTTP_CODE_OK) {
        String responseBody = httpClient.getString();

        if (responseBody.length() > (responseSize - 1)) {
            snprintf_P(error, errorSize, "Response too large! Response length: %d Body start: %s",
                httpClient.getSize(), responseBody.c_str());
        } else {
            snprintf(response, responseSize, responseBody.c_str());
        }
    } else if (httpCode <= 0) {
        snprintf_P(error, errorSize, "Error: %s", httpClient.errorToString(httpCode).c_str());
    } else if (httpCode != HTTP_CODE_OK) {
        snprintf_P(error, errorSize, "Bad HTTP code: %d", httpCode);
    }

    httpClient.end();
    delete wifiClient;

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

HttpPowerMeterClass HttpPowerMeter;
