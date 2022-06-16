#include "WebApi_ws_live.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include <Every.h>

void WebApiWsLiveClass::init(AsyncWebSocket* ws)
{
    _ws = ws;
}

void WebApiWsLiveClass::loop()
{
    EVERY_N_SECONDS(10)
    {
        // do nothing if no WS client is connected
        if (_ws->count() == 0) {
            return;
        }

        DynamicJsonDocument root(40960);
        // Loop all inverters
        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);

            char buffer[sizeof(uint64_t) * 8 + 1];
            sprintf(buffer, "%0lx%08lx",
                ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
                ((uint32_t)(inv->serial() & 0xFFFFFFFF)));

            root[i]["serial"] = String(buffer);
            root[i]["name"] = inv->name();
            root[i]["data_age"] = (millis() - inv->getLastStatsUpdate()) / 1000;
            root[i]["age_critical"] = ((millis() - inv->getLastStatsUpdate()) / 1000) > Configuration.get().Dtu_PollInterval * 5;

            // Loop all channels
            for (uint8_t c = 0; c <= inv->getChannelCount(); c++) {
                addField(root, i, inv, c, FLD_UDC);
                addField(root, i, inv, c, FLD_IDC);
                addField(root, i, inv, c, FLD_PDC);
                addField(root, i, inv, c, FLD_YD);
                addField(root, i, inv, c, FLD_YT);
                addField(root, i, inv, c, FLD_UAC);
                addField(root, i, inv, c, FLD_IAC);
                addField(root, i, inv, c, FLD_PAC);
                addField(root, i, inv, c, FLD_F);
                addField(root, i, inv, c, FLD_T);
                addField(root, i, inv, c, FLD_PCT);
                addField(root, i, inv, c, FLD_EFF);
                addField(root, i, inv, c, FLD_IRR);
            }
        }

        size_t len = measureJson(root);
        AsyncWebSocketMessageBuffer* buffer = _ws->makeBuffer(len); //  creates a buffer (len + 1) for you.
        if (buffer) {
            serializeJson(root, (char*)buffer->get(), len + 1);
            _ws->textAll(buffer);
        }
    }
}

void WebApiWsLiveClass::addField(JsonDocument& root, uint8_t idx, std::shared_ptr<InverterAbstract> inv, uint8_t channel, uint8_t fieldId)
{
    if (inv->hasValue(channel, fieldId)) {
        root[idx][String(channel)][inv->getName(channel, fieldId)]["v"] = inv->getValue(channel, fieldId);
        root[idx][String(channel)][inv->getName(channel, fieldId)]["u"] = inv->getUnit(channel, fieldId);
    }
}