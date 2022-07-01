#include "MqttPublishing.h"
#include "MqttSettings.h"

MqttPublishingClass MqttPublishing;

void MqttPublishingClass::init()
{
}

void MqttPublishingClass::loop()
{
    if (!MqttSettings.getConnected() && Hoymiles.getRadio()->isIdle()) {
        return;
    }

    CONFIG_T& config = Configuration.get();

    if (millis() - _lastPublish > (config.Mqtt_PublishInterval * 1000)) {
        MqttSettings.publish("dtu/uptime", String(millis() / 1000));
        MqttSettings.publish("dtu/ip", WiFi.localIP().toString());

        // Loop all inverters
        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);

            char buffer[sizeof(uint64_t) * 8 + 1];
            sprintf(buffer, "%0lx%08lx",
                ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
                ((uint32_t)(inv->serial() & 0xFFFFFFFF)));
            String subtopic = String(buffer);

            MqttSettings.publish(subtopic + "/name", inv->name());

            uint32_t lastUpdate = inv->getLastStatsUpdate();
            if (lastUpdate > 0 && lastUpdate != _lastPublishStats[i]) {
                _lastPublishStats[i] = lastUpdate;

                // Loop all channels
                for (uint8_t c = 0; c <= inv->getChannelCount(); c++) {
                    publishField(subtopic, inv, c, FLD_UDC);
                    publishField(subtopic, inv, c, FLD_IDC);
                    if (c == 0) {
                        publishField(subtopic, inv, c, FLD_PDC, "dcpower");
                    } else {
                        publishField(subtopic, inv, c, FLD_PDC);
                    }
                    publishField(subtopic, inv, c, FLD_YD);
                    publishField(subtopic, inv, c, FLD_YT);
                    publishField(subtopic, inv, c, FLD_UAC);
                    publishField(subtopic, inv, c, FLD_IAC);
                    publishField(subtopic, inv, c, FLD_PAC);
                    publishField(subtopic, inv, c, FLD_F);
                    publishField(subtopic, inv, c, FLD_T);
                    publishField(subtopic, inv, c, FLD_PCT);
                    publishField(subtopic, inv, c, FLD_EFF);
                    publishField(subtopic, inv, c, FLD_IRR);
                }
            }

            yield();
        }

        _lastPublish = millis();
    }
}

void MqttPublishingClass::publishField(String subtopic, std::shared_ptr<InverterAbstract> inv, uint8_t channel, uint8_t fieldId, String topic)
{
    if (inv->hasChannelFieldValue(channel, fieldId)) {
        String chanName;
        if (topic == "") {
            chanName = inv->getChannelFieldName(channel, fieldId);
        } else {
            chanName = topic;
        }
        chanName.toLowerCase();
        MqttSettings.publish(subtopic + "/" + String(channel) + "/" + chanName, String(inv->getChannelFieldValue(channel, fieldId)));
    }
}