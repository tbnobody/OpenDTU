// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_webapp.h"
#include <MD5Builder.h>

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

void WebApiWebappClass::responseBinaryDataWithETagCache(AsyncWebServerRequest *request, const String &contentType, const String &contentEncoding, const uint8_t *content, size_t len)
{
    auto _md5 = MD5Builder();
    _md5.begin();
    _md5.add(const_cast<uint8_t *>(content), len);
    _md5.calculate();

    String expectedEtag;
    expectedEtag = "\"";
    expectedEtag += _md5.toString();
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
        response = request->beginResponse_P(200, contentType, content, len);
        if (contentEncoding.length() > 0) {
            response->addHeader("Content-Encoding", contentEncoding);
        }
    }

    // HTTP requires cache headers in 200 and 304 to be identical
    response->addHeader("Cache-Control", "public, must-revalidate");
    response->addHeader("ETag", expectedEtag);

    request->send(response);
}

void WebApiWebappClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    _server = &server;

    /*
       We don't validate the request header "Accept-Encoding" if gzip compression is supported!
       We just have the gzipped data available - so we ship them!
    */

    _server->on("/", HTTP_GET, [&](AsyncWebServerRequest* request) {
        responseBinaryDataWithETagCache(request, "text/html", "gzip", file_index_html_start, file_index_html_end - file_index_html_start);
    });

    _server->onNotFound([&](AsyncWebServerRequest* request) {
        responseBinaryDataWithETagCache(request, "text/html", "gzip", file_index_html_start, file_index_html_end - file_index_html_start);
    });

    _server->on("/index.html", HTTP_GET, [&](AsyncWebServerRequest* request) {
        responseBinaryDataWithETagCache(request, "text/html", "gzip", file_index_html_start, file_index_html_end - file_index_html_start);
    });

    _server->on("/favicon.ico", HTTP_GET, [&](AsyncWebServerRequest* request) {
        responseBinaryDataWithETagCache(request, "image/x-icon", "", file_favicon_ico_start, file_favicon_ico_end - file_favicon_ico_start);
    });

    _server->on("/favicon.png", HTTP_GET, [&](AsyncWebServerRequest* request) {
        responseBinaryDataWithETagCache(request, "image/png", "", file_favicon_png_start, file_favicon_png_end - file_favicon_png_start);
    });

    _server->on("/zones.json", HTTP_GET, [&](AsyncWebServerRequest* request) {
        responseBinaryDataWithETagCache(request, "application/json", "gzip", file_zones_json_start, file_zones_json_end - file_zones_json_start);
    });

    _server->on("/site.webmanifest", HTTP_GET, [&](AsyncWebServerRequest* request) {
        responseBinaryDataWithETagCache(request, "application/json", "", file_site_webmanifest_start, file_site_webmanifest_end - file_site_webmanifest_start);
    });

    _server->on("/js/app.js", HTTP_GET, [&](AsyncWebServerRequest* request) {
        responseBinaryDataWithETagCache(request, "text/javascript", "gzip", file_app_js_start, file_app_js_end - file_app_js_start);
    });
}
