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
    PluginsClass() : publisher(msgs) {}
    ~PluginsClass() {}
    void init();
    void loop();
    void subscribeMqtt(Plugin* plugin, char* topic, bool append);
    void ctrlRequest(Plugin* plugin, JsonObject ctrlRequest);
    PluginMessagePublisher& getPublisher();
    //void publishMessage(Plugin* sender, PluginMessage& message);
    void addTimerCb(Plugin* plugin, const char* timername, PLUGIN_TIMER_INTVAL intval, uint32_t interval, std::function<void(void)> timerCb);
    Plugin* getPluginByIndex(int pluginindex);
    Plugin* getPluginById(int pluginid);
    Plugin* getPluginByName(const char* pluginname);
    int getPluginCount();
    void start(Plugin* p);
    
private:
    void addPlugin(Plugin* p);
    void publishToReceiver(std::shared_ptr<PluginMessage> mes);
    void publishToAll(std::shared_ptr<PluginMessage> message);
    void publishInternal();
    void mqttMessageCB(MqttMessage* message);

    uint32_t _lastUpdate = 0;
    bool saveTpSettings = false;
    unsigned int maxnamelen = 0;
    std::vector<Plugin*> plugins;
    typedef struct {
        const char* timername;
        uint32_t interval;
        std::function<void(void)> timerCb;
    } timerentry;
    std::vector<timerentry> timercbs;
    ThreadSafeQueue<std::shared_ptr<PluginMessage>> msgs;
    PluginMessagePublisher publisher;
};

extern PluginsClass Plugins;