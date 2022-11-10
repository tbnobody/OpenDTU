#include "WebApi_prometheus.h"

#include "Configuration.h"
#include "Hoymiles.h"
#include "NetworkSettings.h"

void WebApiPrometheusClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/metrics", HTTP_GET, std::bind(&WebApiPrometheusClass::onPrometheusMetrics, this, _1));
}

void WebApiPrometheusClass::loop()
{
}

void WebApiPrometheusClass::onPrometheusMetrics(AsyncWebServerRequest* request)
{
    auto stream = request->beginResponseStream("text/plain; charset=utf-8", 8192);

    stream->print(F("# HELP opendtu_build Build info\n"));
    stream->print(F("# TYPE opendtu_build gauge\n"));
    stream->printf("opendtu_build{name=\"%s\",id=\"%s\",version=\"%d.%d.%d\"} 1\n",
        NetworkSettings.getHostname().c_str(), AUTO_GIT_HASH, CONFIG_VERSION >> 24 & 0xff, CONFIG_VERSION >> 16 & 0xff, CONFIG_VERSION >> 8 & 0xff);

    stream->print(F("# HELP opendtu_platform Platform info\n"));
    stream->print(F("# TYPE opendtu_platform gauge\n"));
    stream->printf("opendtu_platform{arch=\"%s\",mac=\"%s\"} 1\n", ESP.getChipModel(), WiFi.macAddress().c_str());

    stream->print(F("# HELP opendtu_uptime Uptime in seconds\n"));
    stream->print(F("# TYPE opendtu_uptime counter\n"));
    stream->printf("opendtu_uptime %lld\n", esp_timer_get_time() / 1000000);

    stream->print(F("# HELP opendtu_heap_size System memory size\n"));
    stream->print(F("# TYPE opendtu_heap_size gauge\n"));
    stream->printf("opendtu_heap_size %zu\n", ESP.getHeapSize());

    stream->print(F("# HELP opendtu_free_heap_size System free memory\n"));
    stream->print(F("# TYPE opendtu_free_heap_size gauge\n"));
    stream->printf("opendtu_free_heap_size %zu\n", ESP.getFreeHeap());

    stream->print(F("# HELP wifi_rssi WiFi RSSI\n"));
    stream->print(F("# TYPE wifi_rssi gauge\n"));
    stream->printf("wifi_rssi %d\n", WiFi.RSSI());

    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);

        char serial[sizeof(uint64_t) * 8 + 1];
        snprintf(serial, sizeof(serial), "%0x%08x",
            ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
            ((uint32_t)(inv->serial() & 0xFFFFFFFF)));
        const char* name = inv->name();
        if (i == 0) {
            stream->print(F("# HELP opendtu_last_update last update from inverter in s\n"));
            stream->print(F("# TYPE opendtu_last_update gauge\n"));
        }
        stream->printf("opendtu_last_update{serial=\"%s\",unit=\"%d\",name=\"%s\"} %d\n",
            serial, i, name, inv->Statistics()->getLastUpdate() / 1000);

        // Loop all channels
        for (uint8_t c = 0; c <= inv->Statistics()->getChannelCount(); c++) {
            addField(stream, serial, i, inv, c, FLD_PAC);
            addField(stream, serial, i, inv, c, FLD_UAC);
            addField(stream, serial, i, inv, c, FLD_IAC);
            if (c == 0) {
                addField(stream, serial, i, inv, c, FLD_PDC, "PowerDC");
            } else {
                addField(stream, serial, i, inv, c, FLD_PDC);
            }
            addField(stream, serial, i, inv, c, FLD_UDC);
            addField(stream, serial, i, inv, c, FLD_IDC);
            addField(stream, serial, i, inv, c, FLD_YD);
            addField(stream, serial, i, inv, c, FLD_YT);
            addField(stream, serial, i, inv, c, FLD_F);
            addField(stream, serial, i, inv, c, FLD_T);
            addField(stream, serial, i, inv, c, FLD_PF);
            addField(stream, serial, i, inv, c, FLD_PRA);
            addField(stream, serial, i, inv, c, FLD_EFF);
            addField(stream, serial, i, inv, c, FLD_IRR);
        }
    }
    stream->addHeader(F("Cache-Control"), F("no-cache"));
    request->send(stream);
}

void WebApiPrometheusClass::addField(AsyncResponseStream* stream, const char* serial, uint8_t idx, std::shared_ptr<InverterAbstract> inv, uint8_t channel, uint8_t fieldId, const char* channelName)
{
    if (inv->Statistics()->hasChannelFieldValue(channel, fieldId)) {
        const char* chanName = (channelName == NULL) ? inv->Statistics()->getChannelFieldName(channel, fieldId) : channelName;
        if (idx == 0 && channel == 0) {
            stream->printf("# HELP opendtu_%s in %s\n", chanName, inv->Statistics()->getChannelFieldUnit(channel, fieldId));
            stream->printf("# TYPE opendtu_%s gauge\n", chanName);
        }
        stream->printf("opendtu_%s{serial=\"%s\",unit=\"%d\",name=\"%s\",channel=\"%d\"} %f\n", chanName, serial, idx, inv->name(), channel, inv->Statistics()->getChannelFieldValue(channel, fieldId));
    }
}