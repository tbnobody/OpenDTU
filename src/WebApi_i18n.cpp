// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Thomas Basler and others
 */
#include "WebApi_i18n.h"
#include "I18n.h"
#include "Utils.h"
#include "WebApi.h"
#include <AsyncJson.h>
#include <LittleFS.h>

void WebApiI18nClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/i18n/languages", HTTP_GET, std::bind(&WebApiI18nClass::onI18nLanguages, this, _1));
    server.on("/api/i18n/language", HTTP_GET, std::bind(&WebApiI18nClass::onI18nLanguage, this, _1));
}

void WebApiI18nClass::onI18nLanguages(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse(true);
    auto& root = response->getRoot();
    const auto& languages = I18n.getAvailableLanguages();

    for (auto& language : languages) {
        auto jsonLang = root.add<JsonObject>();

        jsonLang["code"] = language.code;
        jsonLang["name"] = language.name;
    }

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiI18nClass::onI18nLanguage(AsyncWebServerRequest* request)
{
    if (request->hasParam("code")) {
        String code = request->getParam("code")->value();

        String filename = I18n.getFilenameByLocale(code);

        if (filename != "") {
            String md5 = Utils::generateMd5FromFile(filename);

            String expectedEtag;
            expectedEtag = "\"";
            expectedEtag += md5;
            expectedEtag += "\"";

            bool eTagMatch = false;
            if (request->hasHeader("If-None-Match")) {
                const AsyncWebHeader* h = request->getHeader("If-None-Match");
                eTagMatch = h->value().equals(expectedEtag);
            }

            // begin response 200 or 304
            AsyncWebServerResponse* response;
            if (eTagMatch) {
                response = request->beginResponse(304);
            } else {
                response = request->beginResponse(LittleFS, filename, asyncsrv::T_application_json);
            }

            // HTTP requires cache headers in 200 and 304 to be identical
            response->addHeader("Cache-Control", "public, must-revalidate");
            response->addHeader("ETag", expectedEtag);

            request->send(response);
            return;
        }
    }

    request->send(404);
    return;
}
