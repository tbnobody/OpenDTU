// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once


#include "base/plugin.h"
#include "base/pluginmessagepublisher.h"
#include "messages/mqttmessage.h"
#include <cstdint>
#include <ThreadSafeQueue.h>
#include <Every.h>

#define THIRDPARTY_MSG_BUFFERSIZE 1024

class PluginsClass : public System<Plugin> {
public:
    PluginsClass() : publisher(plugins) {}
    ~PluginsClass() {}
    void init();
    void loop();
    void subscribeMqtt(Plugin* plugin, char* topic, bool append);
    void ctrlRequest(Plugin* plugin, JsonObject ctrlRequest);
    PluginMessagePublisher& getPublisher();
    //void publishMessage(Plugin* sender, PluginMessage& message);
    void addTimerCb(Plugin* plugin, const char* timername, PLUGIN_TIMER_INTVAL intval, uint32_t interval, std::function<void(void)> timerCb);
    void removeTimerCb(Plugin* plugin, const char* timername);
    Plugin* getPluginByIndex(int pluginindex);
    Plugin* getPluginById(int pluginid);
    Plugin* getPluginByName(const char* pluginname);
    int getPluginCount();
    void start(Plugin* p);
    
private:
    void addCustomPlugins();
    void addPlugin(std::unique_ptr<Plugin> &p);
    void publishToReceiver(std::shared_ptr<PluginMessage> mes);
    void publishToAll(std::shared_ptr<PluginMessage> message);
    void publishInternal();
    void mqttMessageCB(MqttMessage* message);

    uint32_t _lastUpdate = 0;
    bool saveTpSettings = false;
    unsigned int maxnamelen = 0;
    std::vector<std::unique_ptr<Plugin>> plugins;
    typedef struct {
        const char* timername;
        uint32_t interval;
        std::function<void(void)> timerCb;
        bool valid = true;
    } timerentry;
    std::list<timerentry> timercbs;
    std::list<timerentry> timercbsnew;
    PluginMultiQueueMessagePublisher publisher;
    TaskHandle_t pluginTask;
};

extern PluginsClass Plugins;