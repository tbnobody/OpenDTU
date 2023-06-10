#ifndef __METERPLUGIN_H__
#define __METERPLUGIN_H__

#include "plugin.h"
#include <map>

#ifndef MAX_NUM_METER
#define MAX_NUM_METER 5
#endif

typedef struct {
    String serial;
    float power = 0;
    bool update = false;
} meterstruct;

template <std::size_t N>
class meterarray : public structarray<meterstruct, N> {
    public:
    meterarray() : structarray<meterstruct, N>() {}
    meterstruct* getMeterBySerial(String& serial) {
        return structarray<meterstruct, N>::getByKey([&serial](meterstruct& m){return serial.equals(m.serial);});
    }
    meterstruct* getEmptyIndex() {
        return structarray<meterstruct, N>::getByKey([](meterstruct& s){return s.serial.isEmpty();});
    }
};

class MeterPlugin : public Plugin {
    enum pluginIds { METER_SERIAL,
        METER_POWER };

public:
    MeterPlugin()
        : Plugin(2, "meter")
    {

    }

    void setup() { }
    void loop()
    {
        for (int i = 0; i < meters.size(); i++) {
            if (meters[i].update) {
                meters[i].update = false;
                publishAC(meters[i]);
            }
        }
    }
    meterstruct* getIndex(String& serial)
    {
        meterstruct* ms = meters.getMeterBySerial(serial);
        if(ms==nullptr) {
            ms = meters.getEmptyIndex();
            if(ms!=nullptr) {
                ms->serial = serial;
            }
        }
        return ms;
    }
    void setMeterConsumption(String& serial, float consumption)
    {
        MessageOutput.printf("meterplugin: setMeterConsumption(%s,%f)\n", serial.c_str(),consumption);
        meterstruct* ms = getIndex(serial);
        if (ms==nullptr) {
            MessageOutput.printf("meterplugin: meter[%s] not found!", serial.c_str());
            return;
        }
        ms->power = consumption;
        ms->update = true;
    }
    void mqttCallback(const MqttMessage* message)
    {
        MessageOutput.printf("meterplugin: mqttCallback %s = %s\n", message->topic, (char*)message->payload);
        char buffer[message->length + 1];
        buffer[message->length] = '\0';
        mempcpy(buffer, message->payload, message->length);
        if (topicMap.find(message->topic) != topicMap.end()) {
            String serial = topicMap[message->topic];
            float cons = atof(buffer);
            setMeterConsumption(serial,cons);
        } else {
            DynamicJsonDocument doc(sizeof(buffer));
            DeserializationError error = deserializeJson(doc, buffer);
            if (error) {
                MessageOutput.printf("meterplugin: Failed to deserialize %s\n", buffer);
            } else {
                JsonObject o = doc.as<JsonObject>();
                if (!onRequest(o, o)) {
                    MessageOutput.println("meterplugin: onRequest returned error");
                }
            }
        }
    }

    bool onRequest(JsonObject request, JsonObject response)
    {
        // {"plugin":"meter","meterserial":2394858458,"power":280}
        if (request.containsKey("meterserial") && request.containsKey("power")) {
            String serial = request["meterserial"].as<String>();
            float consumption = request["power"];
            setMeterConsumption(serial, consumption);
            return true;
        } else {
            return false;
        }
    }

    void publishAC(meterstruct& meter)
    {
        MessageOutput.printf("meterplugin: publishPower[%s]: %f W \n", meter.serial.c_str(), meter.power);
        PluginMessage m(*this);
        m.add(FloatValue(METER_POWER, meter.power));
        m.add(StringValue(METER_SERIAL, meter.serial));
        publishMessage(m);
    }
    void saveSettings(JsonObject settings)
    {
        settings[F("meter_mqtt_topics")] = meter_mqtt_topics;
        settings[F("meter_mqtt_json_topics")] = meter_mqtt_json_topics;
    }
    void loadSettings(JsonObject settings)
    {
        if (settings.containsKey(F("meter_mqtt_json_topics"))) {
            meter_mqtt_json_topics = settings[F("meter_mqtt_json_topics")].as<String>();
            initTopics(meter_mqtt_json_topics);
        }
        if (settings.containsKey(F("meter_mqtt_topics"))) {
            meter_mqtt_topics = settings[F("meter_mqtt_topics")].as<String>();
            initTopics(meter_mqtt_topics);
        }
    }
    void mapTopic(String& topic, String& serial)
    {
        topicMap[topic] = serial;
    }
    void initTopics(String& meter_topics)
    {
        char buffer[meter_topics.length()];
        strcpy(buffer, meter_topics.c_str());
        char* token = strtok(buffer, ";");
        while (token != NULL) {
            initTopic(token);
            token = strtok(NULL, ";");
        }
    }
    void initTopic(char* t)
    {
        String buffer = String(t);
        if (buffer.indexOf(':') == -1) {
            subscribeMqtt(t, false);
        } else {
            String topic = buffer.substring(0, buffer.indexOf(':'));
            String sserial = buffer.substring(buffer.indexOf(':') + 1, buffer.length());
            mapTopic(topic, sserial);
            subscribeMqtt((char*)topic.c_str(), false);
        }
    }
    void internalCallback(std::shared_ptr<PluginMessage> message)
    {
        MessageOutput.printf("meterplugin internalCallback from %d\n", message->getSenderId());
    } 
    void internalDataCallback(PluginMessage *message)
    {   
        MessageOutput.printf("meterplugin: internalDataCallback from %d \n", message->getSenderId());
    }
private:
    meterarray<MAX_NUM_METER> meters;
    std::map<String, String> topicMap;
    String meter_mqtt_topics;
    String meter_mqtt_json_topics;
};

#endif /*__METERPLUGIN_H__*/