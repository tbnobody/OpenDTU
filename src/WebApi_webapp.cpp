#include "WebApi_webapp.h"

extern const uint8_t file_index_html_start[] asm("_binary_webapp_dist_index_html_gz_start");
extern const uint8_t file_favicon_ico_start[] asm("_binary_webapp_dist_favicon_ico_start");
extern const uint8_t file_zones_json_start[] asm("_binary_webapp_dist_zones_json_gz_start");
extern const uint8_t file_app_js_start[] asm("_binary_webapp_dist_js_app_js_gz_start");

extern const uint8_t file_index_html_end[] asm("_binary_webapp_dist_index_html_gz_end");
extern const uint8_t file_favicon_ico_end[] asm("_binary_webapp_dist_favicon_ico_end");
extern const uint8_t file_zones_json_end[] asm("_binary_webapp_dist_zones_json_gz_end");
extern const uint8_t file_app_js_end[] asm("_binary_webapp_dist_js_app_js_gz_end");

void WebApiWebappClass::init(AsyncWebServer* server)
{
    using namespace std::placeholders;

    _server = server;

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

    _server->on("/zones.json", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "application/json", file_zones_json_start, file_zones_json_end - file_zones_json_start);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    _server->on("/js/app.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "text/javascript", file_app_js_start, file_app_js_end - file_app_js_start);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
}

void WebApiWebappClass::loop()
{
}