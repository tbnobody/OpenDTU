#pragma once

#include "globals.h"
#include "plugin.h"
#include "pluginmessages.h"
#include "pluginmessagepublisher.h"

class Plugin;
template <class T>
class System
{
public:
    System(){}
    virtual ~System(){}
    virtual void subscribeMqtt(T *plugin, char *topic, bool append) = 0;
    virtual PluginMessagePublisher& getPublisher() = 0;
    //virtual void publishMessage(T *sender, T& message) = 0;
    virtual void addTimerCb(T *plugin, const char* timername, PLUGIN_TIMER_INTVAL intval, uint32_t interval, std::function<void(void)> timerCb) = 0;
    virtual T *getPluginById(int pluginid) = 0;
    virtual T *getPluginByName(const char *pluginname) = 0;
    virtual int getPluginCount() = 0;
};

