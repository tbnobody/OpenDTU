
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */
#include "WebApi_prometheus.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "NetworkSettings.h"
#include "WebApi.h"
#include <Hoymiles.h>

void WebApiPrometheusClass::init(AsyncWebServer& server)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/prometheus/metrics", HTTP_GET, std::bind(&WebApiPrometheusClass::onPrometheusMetricsGet, this, _1));
}

void WebApiPrometheusClass::loop()
{
}

void WebApiPrometheusClass::onPrometheusMetricsGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    try {
        auto stream = request->beginResponseStream("text/plain; charset=utf-8", 40960);

        stream->print("# HELP opendtu_build Build info\n");
        stream->print("# TYPE opendtu_build gauge\n");
        stream->printf("opendtu_build{name=\"%s\",id=\"%s\",version=\"%d.%d.%d\"} 1\n",
            NetworkSettings.getHostname().c_str(), AUTO_GIT_HASH, CONFIG_VERSION >> 24 & 0xff, CONFIG_VERSION >> 16 & 0xff, CONFIG_VERSION >> 8 & 0xff);

        stream->print("# HELP opendtu_platform Platform info\n");
        stream->print("# TYPE opendtu_platform gauge\n");
        stream->printf("opendtu_platform{arch=\"%s\",mac=\"%s\"} 1\n", ESP.getChipModel(), NetworkSettings.macAddress().c_str());

        stream->print("# HELP opendtu_uptime Uptime in seconds\n");
        stream->print("# TYPE opendtu_uptime counter\n");
        stream->printf("opendtu_uptime %lld\n", esp_timer_get_time() / 1000000);

        stream->print("# HELP opendtu_heap_size System memory size\n");
        stream->print("# TYPE opendtu_heap_size gauge\n");
        stream->printf("opendtu_heap_size %zu\n", ESP.getHeapSize());

        stream->print("# HELP opendtu_free_heap_size System free memory\n");
        stream->print("# TYPE opendtu_free_heap_size gauge\n");
        stream->printf("opendtu_free_heap_size %zu\n", ESP.getFreeHeap());

        stream->print("# HELP wifi_rssi WiFi RSSI\n");
        stream->print("# TYPE wifi_rssi gauge\n");
        stream->printf("wifi_rssi %d\n", WiFi.RSSI());

        stream->print("# HELP wifi_station WiFi Station info\n");
        stream->print("# TYPE wifi_station gauge\n");
        stream->printf("wifi_station{bssid=\"%s\"} 1\n", WiFi.BSSIDstr().c_str());

        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);

            String serial = inv->serialString();
            const char* name = inv->name();
            if (i == 0) {
                stream->print("# HELP opendtu_last_update last update from inverter in s\n");
                stream->print("# TYPE opendtu_last_update gauge\n");
            }
            stream->printf("opendtu_last_update{serial=\"%s\",unit=\"%d\",name=\"%s\"} %d\n",
                serial.c_str(), i, name, inv->Statistics()->getLastUpdate() / 1000);

            // Loop all channels if Statistics have been updated at least once since DTU boot
            if (inv->Statistics()->getLastUpdate() > 0) {
                for (auto& t : inv->Statistics()->getChannelTypes()) {
                    for (auto& c : inv->Statistics()->getChannelsByType(t)) {
                        addPanelInfo(stream, serial, i, inv, t, c);
                        for (uint8_t f = 0; f < sizeof(_publishFields) / sizeof(_publishFields[0]); f++) {
                            if (t == TYPE_AC && _publishFields[f].field == FLD_PDC) {
                                addField(stream, serial, i, inv, t, c, _publishFields[f].field, _metricTypes[_publishFields[f].type], "PowerDC");
                            } else {
                                addField(stream, serial, i, inv, t, c, _publishFields[f].field, _metricTypes[_publishFields[f].type]);
                            }
                        }
                    }
                }
            }
        }
        stream->addHeader("Cache-Control", "no-cache");
        request->send(stream);

    } catch (std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Call to /api/prometheus/metrics temporarely out of resources. Reason: \"%s\".\r\n", bad_alloc.what());

        WebApi.sendTooManyRequests(request);
    }
}

void WebApiPrometheusClass::addField(AsyncResponseStream* stream, const String& serial, const uint8_t idx, std::shared_ptr<InverterAbstract> inv, const ChannelType_t type, const ChannelNum_t channel, const FieldId_t fieldId, const char* metricName, const char* channelName)
{
    if (inv->Statistics()->hasChannelFieldValue(type, channel, fieldId)) {
        const char* chanName = (channelName == nullptr) ? inv->Statistics()->getChannelFieldName(type, channel, fieldId) : channelName;
        if (idx == 0 && type == TYPE_AC && channel == 0) {
            stream->printf("# HELP opendtu_%s in %s\n", chanName, inv->Statistics()->getChannelFieldUnit(type, channel, fieldId));
            stream->printf("# TYPE opendtu_%s %s\n", chanName, metricName);
        }
        stream->printf("opendtu_%s{serial=\"%s\",unit=\"%d\",name=\"%s\",type=\"%s\",channel=\"%d\"} %s\n",
            chanName,
            serial.c_str(),
            idx,
            inv->name(),
            inv->Statistics()->getChannelTypeName(type),
            channel,
            inv->Statistics()->getChannelFieldValueString(type, channel, fieldId).c_str());
    }
}

void WebApiPrometheusClass::addPanelInfo(AsyncResponseStream* stream, const String& serial, const uint8_t idx, std::shared_ptr<InverterAbstract> inv, const ChannelType_t type, const ChannelNum_t channel)
{
    if (type != TYPE_DC) {
        return;
    }

    const CONFIG_T& config = Configuration.get();

    const bool printHelp = (idx == 0 && channel == 0);
    if (printHelp) {
        stream->print("# HELP opendtu_PanelInfo panel information\n");
        stream->print("# TYPE opendtu_PanelInfo gauge\n");
    }
    stream->printf("opendtu_PanelInfo{serial=\"%s\",unit=\"%d\",name=\"%s\",channel=\"%d\",panelname=\"%s\"} 1\n",
        serial.c_str(),
        idx,
        inv->name(),
        channel,
        config.Inverter[idx].channel[channel].Name);

    if (printHelp) {
        stream->print("# HELP opendtu_MaxPower panel maximum output power\n");
        stream->print("# TYPE opendtu_MaxPower gauge\n");
    }
    stream->printf("opendtu_MaxPower{serial=\"%s\",unit=\"%d\",name=\"%s\",channel=\"%d\"} %d\n",
        serial.c_str(),
        idx,
        inv->name(),
        channel,
        config.Inverter[idx].channel[channel].MaxChannelPower);

    if (printHelp) {
        stream->print("# HELP opendtu_YieldTotalOffset panel yield offset (for used inverters)\n");
        stream->print("# TYPE opendtu_YieldTotalOffset gauge\n");
    }
    stream->printf("opendtu_YieldTotalOffset{serial=\"%s\",unit=\"%d\",name=\"%s\",channel=\"%d\"} %f\n",
        serial.c_str(),
        idx,
        inv->name(),
        channel,
        config.Inverter[idx].channel[channel].YieldTotalOffset);
}
