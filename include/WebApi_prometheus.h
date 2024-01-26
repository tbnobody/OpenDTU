// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <Hoymiles.h>
#include <TaskSchedulerDeclarations.h>
#include <map>

class WebApiPrometheusClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onPrometheusMetricsGet(AsyncWebServerRequest* request);

    void addField(AsyncResponseStream* stream, const String& serial, const uint8_t idx, std::shared_ptr<InverterAbstract> inv, const ChannelType_t type, const ChannelNum_t channel, const FieldId_t fieldId, const char* metricName, const char* channelName = nullptr);

    void addPanelInfo(AsyncResponseStream* stream, const String& serial, const uint8_t idx, std::shared_ptr<InverterAbstract> inv, const ChannelType_t type, const ChannelNum_t channel);

    AsyncWebServer* _server;

    enum MetricType_t {
        NONE = 0,
        GAUGE,
        COUNTER,
    };
    const char* _metricTypes[3] = { 0, "gauge", "counter" };

    struct publish_type_t {
        FieldId_t field;
        MetricType_t type;
    };

    const publish_type_t _publishFields[14] = {
        { FLD_PAC, MetricType_t::GAUGE },
        { FLD_UAC, MetricType_t::GAUGE },
        { FLD_IAC, MetricType_t::GAUGE },
        { FLD_PDC, MetricType_t::GAUGE },
        { FLD_UDC, MetricType_t::GAUGE },
        { FLD_IDC, MetricType_t::GAUGE },
        { FLD_YD, MetricType_t::COUNTER },
        { FLD_YT, MetricType_t::COUNTER },
        { FLD_F, MetricType_t::GAUGE },
        { FLD_T, MetricType_t::GAUGE },
        { FLD_PF, MetricType_t::GAUGE },
        { FLD_Q, MetricType_t::GAUGE },
        { FLD_EFF, MetricType_t::GAUGE },
        { FLD_IRR, MetricType_t::GAUGE },
    };
};
