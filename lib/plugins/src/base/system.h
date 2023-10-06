#pragma once

#include "globals.h"
#include "plugin.h"
#include "pluginmessagepublisher.h"
#include "pluginmessages.h"

class Plugin;
template <class T> class SystemConfigurator {
public:
  SystemConfigurator() {}
  virtual ~SystemConfigurator() {}
  virtual void readConfig(T* t) {}
  virtual void saveConfig(T* t) {}
};

template <class T> class System {
public:
  System() {}
  virtual ~System() {}
  virtual void setConfigurator(SystemConfigurator<T> configurator);
  virtual void subscribeMqtt(T *plugin, char *topic, bool append) = 0;
  virtual void receiveMqtt(T *plugin, const char *topic, const uint8_t *data,
                           size_t len) = 0;
  virtual PluginMessagePublisher &getPublisher() = 0;
  virtual void addTimerCb(T *plugin, const char *timername,
                          PLUGIN_TIMER_INTVAL intval, uint32_t interval,
                          std::function<void(void)> timerCb) = 0;
  virtual bool hasTimerCb(T *plugin, const char *timername) = 0;
  virtual void removeTimerCb(T *plugin, const char *timername) = 0;
  virtual T *getPluginById(int pluginid) = 0;
  virtual T *getPluginByName(const char *pluginname) = 0;
  virtual int getPluginCount() = 0;
};
