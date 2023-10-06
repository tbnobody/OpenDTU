#pragma once

#include "HoymilesPlugin.h"
#include "MeterPlugin.h"
#include "PowercontrolPlugin.h"
#include "PublishPlugin.h"
#include "base/PluginDebug.h"
#include "base/plugin.h"
#include "base/system.h"
#include "develsupportplugin.h"
#include "messages/mqttmessage.h"
#include <list>
#if __has_include("customplugin.h")
#include "customplugin.h"
#endif

#include "PluginSystem.hpp"

class PluginSystem : public System<Plugin> {
public:
  PluginSystem(SystemConfigurator<Plugin>& configurator);
  ~PluginSystem() {}
  void setMqttSubscribeCb(
      std::function<void(int id, const char *topic, bool append)> subscribeCb);
  void
  setMqttPublishCb(std::function<void(int id, const char *topic, uint8_t *data,
                                      size_t len, bool append)>
                       publishCb);
  void init();

  void mqttMessageCB(MqttMessage *message);

  void loop();
  void start(Plugin *p);

  void subscribeMqtt(Plugin *plugin, char *topic, bool append);
  void receiveMqtt(Plugin *plugin, const char *topic, const uint8_t *data,
                   size_t len);

  void addTimerCb(Plugin *plugin, const char *timername,
                  PLUGIN_TIMER_INTVAL intval, uint32_t interval,
                  std::function<void(void)> timerCb);
  void removeTimerCb(Plugin *plugin, const char *timername);

  bool hasTimerCb(Plugin *plugin, const char *timername);
  Plugin *getPluginByIndex(int pluginindex);
  Plugin *getPluginById(int pluginid);
  Plugin *getPluginByName(const char *pluginname);
  int getPluginCount();
  void setConfigurator(SystemConfigurator<Plugin> c);
  void publishInternal();
  PluginMessagePublisher &getPublisher();

protected:
  virtual void
  addCustomPlugins(std::vector<std::unique_ptr<Plugin>> &pluginsvec);

private:
  unsigned int maxnamelen = 0;
  std::function<void(int id, char *topic, bool append)> mqttSubscribeCB =
      nullptr;
  std::function<void(int id, const char *topic, uint8_t *date, size_t len,
                     bool append)>
      mqttPublishCB = nullptr;

  std::vector<std::unique_ptr<Plugin>> plugins;
  typedef struct {
    const char *timername;
    uint32_t interval;
    std::function<void(void)> timerCb;
    bool valid = true;
  } timerentry;
  std::list<timerentry> timercbs;
  std::list<timerentry> timercbsnew;
  PluginMultiQueueMessagePublisher publisher;
  SystemConfigurator<Plugin>& configurator;
};