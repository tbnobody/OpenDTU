#include "WebApi_ws_live.h"
#include "AsyncJson.h"
#include "Configuration.h"

void WebApiWsLiveClass::init(AsyncWebSocket* ws)
{
    _ws = ws;
}

void WebApiWsLiveClass::loop()
{
    // do nothing if no WS client is connected
    if (_ws->count() == 0) {
        return;
    }

    if (millis() - _lastInvUpdateCheck < 1000) {
        return;
    }
    _lastInvUpdateCheck = millis();

    uint32_t maxTimeStamp = 0;
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);

        if (inv->getLastStatsUpdate() > maxTimeStamp) {
            maxTimeStamp = inv->getLastStatsUpdate();
        }
    }

    // Update on every inverter change or at least after 10 seconds
    if (millis() - _lastWsPublish > (10 * 1000) || (maxTimeStamp != _newestInverterTimestamp)) {

        DynamicJsonDocument root(40960);
        // Loop all inverters
        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);

            char buffer[sizeof(uint64_t) * 8 + 1];
            sprintf(buffer, "%0lx%08lx",
                ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
                ((uint32_t)(inv->serial() & 0xFFFFFFFF)));

            root[i][F("serial")] = String(buffer);
            root[i][F("name")] = inv->name();
            root[i][F("data_age")] = (millis() - inv->getLastStatsUpdate()) / 1000;
            root[i][F("age_critical")] = ((millis() - inv->getLastStatsUpdate()) / 1000) > Configuration.get().Dtu_PollInterval * 5;

            // Loop all channels
            for (uint8_t c = 0; c <= inv->getChannelCount(); c++) {
                addField(root, i, inv, c, FLD_PAC);
                addField(root, i, inv, c, FLD_UAC);
                addField(root, i, inv, c, FLD_IAC);
                if (c == 0) {
                    addField(root, i, inv, c, FLD_PDC, F("Power DC"));
                } else {
                    addField(root, i, inv, c, FLD_PDC);
                }
                addField(root, i, inv, c, FLD_UDC);
                addField(root, i, inv, c, FLD_IDC);
                addField(root, i, inv, c, FLD_YD);
                addField(root, i, inv, c, FLD_YT);
                addField(root, i, inv, c, FLD_F);
                addField(root, i, inv, c, FLD_T);
                addField(root, i, inv, c, FLD_PCT);
                addField(root, i, inv, c, FLD_EFF);
                addField(root, i, inv, c, FLD_IRR);
            }

            if (inv->hasChannelFieldValue(CH0, FLD_EVT_LOG)) {
                root[i][F("events")] = inv->EventLog()->getEntryCount();
            } else {
                root[i][F("events")] = -1;
            }

            if (inv->getLastStatsUpdate() > _newestInverterTimestamp) {
                _newestInverterTimestamp = inv->getLastStatsUpdate();
            }
        }

        size_t len = measureJson(root);
        AsyncWebSocketMessageBuffer* buffer = _ws->makeBuffer(len); //  creates a buffer (len + 1) for you.
        if (buffer) {
            serializeJson(root, (char*)buffer->get(), len + 1);
            _ws->textAll(buffer);
        }

        _lastWsPublish = millis();
    }
}

void WebApiWsLiveClass::addField(JsonDocument& root, uint8_t idx, std::shared_ptr<InverterAbstract> inv, uint8_t channel, uint8_t fieldId, String topic)
{
    if (inv->hasChannelFieldValue(channel, fieldId)) {
        String chanName;
        if (topic == "") {
            chanName = inv->getChannelFieldName(channel, fieldId);
        } else {
            chanName = topic;
        }
        root[idx][String(channel)][chanName]["v"] = inv->getChannelFieldValue(channel, fieldId);
        root[idx][String(channel)][chanName]["u"] = inv->getChannelFieldUnit(channel, fieldId);
    }
}