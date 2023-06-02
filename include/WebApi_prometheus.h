// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <Hoymiles.h>
#include <map>

class WebApiPrometheusClass {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onPrometheusMetricsGet(AsyncWebServerRequest* request);

    void addField(AsyncResponseStream* stream, String& serial, uint8_t idx, std::shared_ptr<InverterAbstract> inv, ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId, const char* channelName = NULL);

    void addPanelInfo(AsyncResponseStream* stream, String& serial, uint8_t idx, std::shared_ptr<InverterAbstract> inv, ChannelType_t type, ChannelNum_t channel);

    AsyncWebServer* _server;

    enum {
        METRIC_TYPE_NONE = 0,
        METRIC_TYPE_GAUGE,
        METRIC_TYPE_COUNTER,
    };
    const char* _metricTypes[3] = { 0, "gauge", "counter" };

    std::map<FieldId_t, uint8_t> _fieldMetricAssignment {
        { FLD_UDC, METRIC_TYPE_GAUGE },
        { FLD_IDC, METRIC_TYPE_GAUGE },
        { FLD_PDC, METRIC_TYPE_GAUGE },
        { FLD_YD, METRIC_TYPE_COUNTER },
        { FLD_YT, METRIC_TYPE_COUNTER },
        { FLD_UAC, METRIC_TYPE_GAUGE },
        { FLD_IAC, METRIC_TYPE_GAUGE },
        { FLD_PAC, METRIC_TYPE_GAUGE },
        { FLD_F, METRIC_TYPE_GAUGE },
        { FLD_T, METRIC_TYPE_GAUGE },
        { FLD_PF, METRIC_TYPE_GAUGE },
        { FLD_EFF, METRIC_TYPE_GAUGE },
        { FLD_IRR, METRIC_TYPE_GAUGE },
        { FLD_Q, METRIC_TYPE_GAUGE }
    };
};