// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "plugin.h"
#include <cstdint>
#include <queue>
#include <Every.h>

#define THIRDPARTY_MSG_BUFFERSIZE 1024

class PluginsClass : public System {
public:
    void init();
    void loop();
    void subscribeMqtt(Plugin* plugin, char* topic, bool append);
    void ctrlRequest(Plugin* plugin, JsonObject ctrlRequest);
    bool enqueueMessage(Plugin* sender, char* topic, char* data, bool append);
    void publishMessage(Plugin* sender, PluginMessage& message);
    void addTimerCb(Plugin* plugin, const char* timername, PLUGIN_TIMER_INTVAL intval, uint32_t interval, std::function<void(void)> timerCb);
    Plugin* getPluginByIndex(int pluginindex);
    Plugin* getPluginById(int pluginid);
    Plugin* getPluginByName(const char* pluginname);
    int getPluginCount();
    void start(Plugin* p);
    
private:
    void addPlugin(Plugin* p);
    void publishToReceiver(PluginMessage* mes);
    void publishToAll(PluginMessage* message);
    void publishInternal();
    void publish();

    uint32_t _lastUpdate = 0;

    bool saveTpSettings = false;
    char buffer[THIRDPARTY_MSG_BUFFERSIZE];
    uint16_t bufferindex = 0;
    typedef struct
    {
        int pluginid;
        uint16_t topicindex;
        uint16_t dataindex;
        bool appendtopic;
    } qentry;
    std::queue<qentry> q;
    unsigned int maxnamelen = 0;
    std::vector<Plugin*> plugins;
    typedef struct {
        const char* timername;
        uint32_t interval;
        std::function<void(void)> timerCb;
    } timerentry;
    std::vector<timerentry> timercbs;
    std::queue<std::shared_ptr<PluginMessage>> msgs;
};

extern PluginsClass Plugins;