// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Tobias Weidelt
 */
#include "WebApi_fronius.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "SunspecApi.h"
#include "WebApi.h"
#include <AsyncJson.h>
#include <WiFiUdp.h>

WiFiUDP Udp;

void WebApiFroniusClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/solar_api/v1/GetActiveDeviceInfo.cgi", HTTP_GET, std::bind(&WebApiFroniusClass::onGetActiveDeviceInfo, this, _1));
    _server->on("/solar_api/v1/GetInverterInfo.cgi", HTTP_GET, std::bind(&WebApiFroniusClass::onGetInverterInfo, this, _1));
    _server->onNotFound(std::bind(&WebApiFroniusClass::NotFound, this, _1));

    Udp.begin(50049); // local port to listen on
}

void WebApiFroniusClass::loop()
{
    // handle Victron UDP autodection for Fronius Solar API

    uint8_t incomingPacket[255]; // buffer for incoming packets
    uint8_t replyPacket[] = "Hello"; // a reply string to send back

    int packetSize = Udp.parsePacket();
    if (packetSize) {
        // receive incoming UDP packets
        int len = Udp.read(incomingPacket, 254);
        if (len >= 0) {
            incomingPacket[len] = 0;
        }

        // send back a reply, to the IP address and port we got the packet from
        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        Udp.write(replyPacket, sizeof(replyPacket));
        Udp.endPacket();
    }
}

void WebApiFroniusClass::onGetActiveDeviceInfo(AsyncWebServerRequest* request)
{
    MessageOutput.println("WebApiFroniusClass::onGetActiveDeviceInfo");

    // XXX check credentials?

    AsyncBasicResponse* response = new AsyncBasicResponse(200, "application/json", "{\"Head\":{\"Status\":{\"Code\":0}},\"Body\":{\"Data\":{\"126\":{\"DT\":81,\"Serial\":\"116480155000\"},\"127\":{\"DT\":81,\"Serial\":\"116480424353\"},\"128\":{\"DT\":81,\"Serial\":\"116480423039\"},\"129\":{\"DT\":81,\"Serial\":\"116480423655\"}}}}");
    response->addHeader("Connection", "keep-alive");
    request->send(response);
    request->client()->close(true);
    return;

    try {
        AsyncJsonResponse* response = new AsyncJsonResponse(false, 4096U);
        JsonObject root = response->getRoot();

        JsonObject head = root.createNestedObject("Head");
        JsonObject status = head.createNestedObject("Status");
        status["Code"] = 0; // 0 = OK

        JsonObject body = root.createNestedObject("Body");
        JsonObject data = body.createNestedObject("Data");

        auto deviceClass = request->getParam("DeviceClass");
        if (deviceClass && deviceClass->value() == "Inverter") {
            MessageOutput.println("WebApiFroniusClass::onGetActiveDeviceInfo - Inverter");

            const CONFIG_T& config = Configuration.get();

            for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
                auto inv = config.Inverter[i];
                if (inv.Serial > 0) {
                    auto unitId = SunspecConstants::BASE_UNIT_ID + i;
                    MessageOutput.println("WebApiFroniusClass::onGetActiveDeviceInfo - Inverter unit ID " + String(unitId));
                    JsonObject obj = data.createNestedObject(String(unitId));

                    obj["DT"] = FRONIUS_INVERTER_TYPE_1PHASE;
                    // Inverter Serial is read as HEX
                    char buffer[sizeof(uint64_t) * 8 + 1];
                    snprintf(buffer, sizeof(buffer), "%0x%08x",
                        ((uint32_t)((inv.Serial >> 32) & 0xFFFFFFFF)),
                        ((uint32_t)(inv.Serial & 0xFFFFFFFF)));
                    obj["Serial"] = buffer;
                }
            }

        } else {
            MessageOutput.println("WebApiFroniusClass::onGetActiveDeviceInfo - Parameter DeviceClass=Inverter missing");
        }

        response->setLength();
        request->send(response);

    } catch (const std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Call to %s temporarely out of resources. Reason: \"%s\".\r\n", request->url().c_str(), bad_alloc.what());
        WebApi.sendTooManyRequests(request);
    } catch (const std::exception& exc) {
        MessageOutput.printf("Unknown exception in %s. Reason: \"%s\".\r\n", request->url().c_str(), exc.what());
        WebApi.sendTooManyRequests(request);
    }
}

void WebApiFroniusClass::onGetInverterInfo(AsyncWebServerRequest* request)
{
    MessageOutput.println("WebApiFroniusClass::onGetInverterInfo");

    // XXX check credentials?

    AsyncBasicResponse* response = new AsyncBasicResponse(200, "application/json", "{\"Head\":{\"Status\":{\"Code\":0}},\"Body\":{\"Data\":{\"126\":{\"DT\":81,\"UniqueID\":\"116480155000\",\"CustomName\":\"HMS-2000-4T_1\",\"ErrorCode\":0,\"StatusCode\":7},\"127\":{\"DT\":81,\"UniqueID\":\"116480424353\",\"CustomName\":\"HMS-2000-4T_2\",\"ErrorCode\":0,\"StatusCode\":7},\"128\":{\"DT\":81,\"UniqueID\":\"116480423039\",\"CustomName\":\"HMS-2000-4T_3\",\"ErrorCode\":0,\"StatusCode\":7},\"129\":{\"DT\":81,\"UniqueID\":\"116480423655\",\"CustomName\":\"HMS-2000-4T_4\",\"ErrorCode\":0,\"StatusCode\":7}}}}");
    response->addHeader("Connection", "keep-alive");
    request->send(response);
    request->client()->close(true);
    return;

    try {
        AsyncJsonResponse* response = new AsyncJsonResponse(false, 4096U);
        JsonObject root = response->getRoot();

        JsonObject head = root.createNestedObject("Head");
        JsonObject status = head.createNestedObject("Status");
        status["Code"] = 0; // 0 = OK

        JsonObject body = root.createNestedObject("Body");
        JsonObject data = body.createNestedObject("Data");

        const CONFIG_T& config = Configuration.get();

        for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
            auto inv = config.Inverter[i];
            if (inv.Serial > 0) {
                auto unitId = SunspecConstants::BASE_UNIT_ID + i;
                MessageOutput.println("WebApiFroniusClass::onGetInverterInfo - Inverter unit ID " + String(unitId));

                JsonObject obj = data.createNestedObject(String(unitId));
                obj["DT"] = FRONIUS_INVERTER_TYPE_1PHASE;

                // Inverter Serial is read as HEX
                char buffer[sizeof(uint64_t) * 8 + 1];
                snprintf(buffer, sizeof(buffer), "%0x%08x",
                    ((uint32_t)((inv.Serial >> 32) & 0xFFFFFFFF)),
                    ((uint32_t)(inv.Serial & 0xFFFFFFFF)));
                obj["UniqueID"] = buffer;
                obj["CustomName"] = inv.Name; // user defined name, may be left empty
                obj["ErrorCode"] = 0; // 0 = No error
                obj["StatusCode"] = 7; // 7 = Running
            }
        }

        response->setLength();
        request->send(response);

    } catch (const std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Call to %s temporarely out of resources. Reason: \"%s\".\r\n", request->url().c_str(), bad_alloc.what());
        WebApi.sendTooManyRequests(request);
    } catch (const std::exception& exc) {
        MessageOutput.printf("Unknown exception in %s. Reason: \"%s\".\r\n", request->url().c_str(), exc.what());
        WebApi.sendTooManyRequests(request);
    }
}

void WebApiFroniusClass::NotFound(AsyncWebServerRequest* request)
{
    MessageOutput.println("WebApiFroniusClass::NotFound - " + request->url());

    AsyncBasicResponse* response = new AsyncBasicResponse(404, "text/plain", "Not found: " + request->url());
    request->send(response);
}
