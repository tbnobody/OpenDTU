#ifndef __DEMOPLUGIN_H__
#define __DEMOPLUGIN_H__

#include "plugin.h"

class demoPlugin : public Plugin
{
    enum demoPluginIds {SOMEVALUE,SOMEOTHERVALUE};

public:
    demoPlugin() : Plugin(999, "demo") {}
    void setup()
    {

    }
    void onTickerSetup() {
        MessageOutput.println("demoplugin:onTickerSetup()");
        /*
        addTimerCb(SECOND, 3, [this]() { 
            PluginMessage m(*this);
            m.addList(FloatValue(SOMEVALUE, (float)42.0),StringValue(SOMEOTHERVALUE, "blah blub")); 
            publishMessage(m);
        },"demoplugintimer1");
        addTimerCb(SECOND, 4, [this]() {
            enqueueMessage((char*)"out",(char*)"hello world!",false);
            enqueueMessage((char*)"out",(char*)"hello world!"); 
        },"demoplugintimer2");
        */
        if(debugHeap) {
            addTimerCb(SECOND, 10, [this]() {
                MessageOutput.printf("demoplugin: free heap: %d\n" ,ESP.getFreeHeap());
            },"debugHeapTimer");
        }
    }
    void loop()
    {
        // main loop
    }
    void inverterCallback(const InverterMessage *message)
    {
        // receice inverter data
    }

    void onMqttSubscribe() {
        subscribeMqtt((char*)"public/topic",false);
    }

    void mqttCallback(const MqttMessage *message)
    {
        // receive data for
        // ahoi topic: 'DEF_MQTT_TOPIC/devcontrol/#'
        // thirdparty topic: 'DEF_MQTT_TOPIC/thirdparty/#'
        // default for DEF_MQTT_TOPIC = "inverter" (see config.h)
       MessageOutput.printf("demoplugin: mqttCallback %s = %s\n",message->topic, (char*)message->payload);
    }

    void internalDataCallback(PluginMessage *message)
    {   
        if(debugPluginMessages) {
            DBGPRINTMESSAGELN(DBG_INFO,"demoplugin",message);
            DBGPRINTMESSAGETAGSLN(DBG_INFO,message);
            DBGPRINTMESSAGERUNTIMELN(DBG_INFO,message);
        }
    }

    bool onRequest(JsonObject request, JsonObject response) { 
        response[F("someoutput")]=millis();
        return true; 
    }

    void saveSettings(JsonObject settings) {
        settings[F("debugHeap")]=debugHeap;
        settings[F("debugPluginMessages")]=debugPluginMessages;
        settings[F("booleansetting")]=booleansetting;
        settings[F("floatsetting")]=floatsetting;
        settings[F("stringsetting")]=stringsetting;
    }
    void loadSettings(JsonObject settings) {
        if(settings.containsKey(F("booleansetting")))
            booleansetting=settings[F("booleansetting")];
        if(settings.containsKey(F("floatsetting")))
            floatsetting=settings[F("floatsetting")];
        if(settings.containsKey(F("stringsetting")))
            settings[F("stringsetting")].as<String>().toCharArray(stringsetting,sizeof(stringsetting));
        if(settings.containsKey(F("debugHeap")))
            debugHeap=settings[F("debugHeap")];
         if(settings.containsKey(F("debugPluginMessages")))
            debugPluginMessages=settings[F("debugPluginMessages")];
    }
    private:
    bool debugHeap = false;
    bool debugPluginMessages = false;
    bool booleansetting = false;
    float floatsetting = 23.0;
    char stringsetting[32] = "some default string";
};

#endif /*__DEMOPLUGIN_H__*/