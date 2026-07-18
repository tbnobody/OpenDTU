// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2026 Thomas Basler and others
 */
#include "WebApi_devinfo.h"
#include "WebApi.h"
#include <AsyncJson.h>
#include <HmsWifiInverter.h>
#include <Hoymiles.h>
#include <ctime>

void WebApiDevInfoClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/devinfo/status", HTTP_GET, static_cast<ArRequestHandlerFunction>(std::bind(&WebApiDevInfoClass::onDevInfoStatus, this, _1)));
}

void WebApiDevInfoClass::onDevInfoStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    auto serial = WebApi.parseSerialFromRequest(request);
    auto inv = Hoymiles.getInverterBySerial(serial);

    if (inv != nullptr) {
        root["valid_data"] = inv->DevInfo()->getLastUpdate() > 0;
        root["fw_bootloader_version"] = inv->DevInfo()->getFwBootloaderVersion();
        root["fw_build_version"] = inv->DevInfo()->getFwBuildVersion();
        root["hw_part_number"] = inv->DevInfo()->getHwPartNumber();
        root["hw_version"] = inv->DevInfo()->getHwVersion();
        root["max_power"] = inv->DevInfo()->getMaxPower();
        root["fw_build_datetime"] = inv->DevInfo()->getFwBuildDateTimeStr();
        root["pdl_supported"] = inv->supportsPowerDistributionLogic();

        // HW part number lookup gives the exact model name for RF inverters.
        // WiFi inverters also have pv_hw_pn from AppInfo which feeds the same
        // lookup table. Fall back to the serial-derived model name if the HW
        // part number isn't in the table.
        String modelName = inv->DevInfo()->getHwModelName();
        if (modelName.isEmpty() && inv->isWifiInverter()) {
            auto* wifiInv = static_cast<HmsWifiInverter*>(inv.get());
            const String& wifiModel = wifiInv->wifiModelName();
            modelName = wifiModel.isEmpty() ? inv->typeName() : wifiModel;
        }
        root["hw_model_name"] = modelName;
    }

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}
