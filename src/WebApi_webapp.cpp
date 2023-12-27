// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */
#include "WebApi_webapp.h"

extern const uint8_t file_index_html_start[] asm("_binary_webapp_dist_index_html_gz_start");
extern const uint8_t file_favicon_ico_start[] asm("_binary_webapp_dist_favicon_ico_start");
extern const uint8_t file_favicon_png_start[] asm("_binary_webapp_dist_favicon_png_start");
extern const uint8_t file_zones_json_start[] asm("_binary_webapp_dist_zones_json_gz_start");
extern const uint8_t file_app_js_start[] asm("_binary_webapp_dist_js_app_js_gz_start");
extern const uint8_t file_site_webmanifest_start[] asm("_binary_webapp_dist_site_webmanifest_start");

extern const uint8_t file_index_html_end[] asm("_binary_webapp_dist_index_html_gz_end");
extern const uint8_t file_favicon_ico_end[] asm("_binary_webapp_dist_favicon_ico_end");
extern const uint8_t file_favicon_png_end[] asm("_binary_webapp_dist_favicon_png_end");
extern const uint8_t file_zones_json_end[] asm("_binary_webapp_dist_zones_json_gz_end");
extern const uint8_t file_app_js_end[] asm("_binary_webapp_dist_js_app_js_gz_end");
extern const uint8_t file_site_webmanifest_end[] asm("_binary_webapp_dist_site_webmanifest_end");

#ifdef AUTO_GIT_HASH
#define ETAG_HTTP_HEADER_VAL "\"" AUTO_GIT_HASH "\"" // ETag value must be between quotes
#endif

void WebApiWebappClass::init(AsyncWebServer& server)
{
    _server = &server;

    _server->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", file_index_html_start, file_index_html_end - file_index_html_start);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    _server->onNotFound([](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", file_index_html_start, file_index_html_end - file_index_html_start);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    _server->on("/index.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", file_index_html_start, file_index_html_end - file_index_html_start);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    _server->on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "image/x-icon", file_favicon_ico_start, file_favicon_ico_end - file_favicon_ico_start);
        request->send(response);
    });

    _server->on("/favicon.png", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "image/png", file_favicon_png_start, file_favicon_png_end - file_favicon_png_start);
        request->send(response);
    });

    _server->on("/zones.json", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "application/json", file_zones_json_start, file_zones_json_end - file_zones_json_start);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    _server->on("/site.webmanifest", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "application/json", file_site_webmanifest_start, file_site_webmanifest_end - file_site_webmanifest_start);
        request->send(response);
    });

    _server->on("/js/app.js", HTTP_GET, [](AsyncWebServerRequest* request) {
#ifdef ETAG_HTTP_HEADER_VAL
        // check client If-None-Match header vs ETag/AUTO_GIT_HASH
        bool eTagMatch = false;
        if (request->hasHeader("If-None-Match")) {
            const AsyncWebHeader* h = request->getHeader("If-None-Match");
            if (strncmp(ETAG_HTTP_HEADER_VAL, h->value().c_str(), strlen(ETAG_HTTP_HEADER_VAL)) == 0) {
                eTagMatch = true;
            }
        }

        // begin response 200 or 304
        AsyncWebServerResponse* response;
        if (eTagMatch) {
            response = request->beginResponse(304);
        } else {
            response = request->beginResponse_P(200, "text/javascript", file_app_js_start, file_app_js_end - file_app_js_start);
            response->addHeader("Content-Encoding", "gzip");
        }
        // HTTP requires cache headers in 200 and 304 to be identical
        response->addHeader("Cache-Control", "public, must-revalidate");
        response->addHeader("ETag", ETAG_HTTP_HEADER_VAL);
#else
        AsyncWebServerResponse* response = request->beginResponse_P(200, "text/javascript", file_app_js_start, file_app_js_end - file_app_js_start);
        response->addHeader("Content-Encoding", "gzip");
#endif
        request->send(response);
    });
}

void WebApiWebappClass::loop()
{
}
