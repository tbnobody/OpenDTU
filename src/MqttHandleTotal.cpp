// #350 Publish Inverter Total Data via MQTT
#include "MqttHandleTotal.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include <Hoymiles.h>

MqttHandleTotalClass MqttHandleTotal;

void MqttHandleTotalClass::init()
{
}

void MqttHandleTotalClass::loop()
{
    if (!MqttSettings.getConnected() || !Hoymiles.getRadio()->isIdle()) {
        return;
    }
    float totalPower = 0;
    float totalYieldDay = 0;
    float totalYieldTotal = 0;

    const CONFIG_T& config = Configuration.get();

    if (millis() - _lastPublish > (config.Mqtt_PublishInterval * 1000)) {
        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);
            for (auto& c : inv->Statistics()->getChannelsByType(TYPE_AC)) {
                totalPower += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_PAC);
                totalYieldDay += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_YD);
                totalYieldTotal += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_YT);
            }
        }

        MqttSettings.publish("total/Power", String(totalPower));
        MqttSettings.publish("total/YieldDay", String(totalYieldDay));
        MqttSettings.publish("total/YieldTotal", String(totalYieldTotal));
        _lastPublish = millis();
    }
}