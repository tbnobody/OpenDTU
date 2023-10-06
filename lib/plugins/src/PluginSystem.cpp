

#include "HoymilesPlugin.h"
#include "MeterPlugin.h"
#include "PowercontrolPlugin.h"
#include "PublishPlugin.h"
#include "base/PluginDebug.h"
#include "base/plugin.h"
#include "base/system.h"
#include "develsupportplugin.h"
#include "messages/mqttmessage.h"
#include <Every.h>
#include <list>
#if __has_include("customplugin.h ")
#include "customplugin.h"
#endif

#include "PluginSystem.hpp"

PluginSystem::PluginSystem(SystemConfigurator<Plugin>& _configurator)
    : System<Plugin>(), publisher(plugins), configurator(_configurator) {}
void PluginSystem::setMqttSubscribeCb(
    std::function<void(int id, const char *topic, bool append)> subscribeCB) {
  mqttSubscribeCB = subscribeCB;
}
void PluginSystem::setMqttPublishCb(
    std::function<void(int id, const char *topic, uint8_t *data, size_t len,
                       bool append)>
        publishCB) {
  mqttPublishCB = publishCB;
}
void PluginSystem::setConfigurator(SystemConfigurator<Plugin> c) {
  configurator = c;
}
void PluginSystem::init() {
  plugins.push_back(std::make_unique<DevelSupportPlugin>(DevelSupportPlugin()));
  plugins.push_back(std::make_unique<HoymilesPlugin>(HoymilesPlugin()));
  plugins.push_back(std::make_unique<MeterPlugin>(MeterPlugin()));
  plugins.push_back(std::make_unique<PowercontrolPlugin>(PowercontrolPlugin()));
  PublishPlugin publishP = PublishPlugin();
  publishP.mqttMessageCB(
      std::bind(&PluginSystem::mqttMessageCB, this, std::placeholders::_1));
  plugins.push_back(std::make_unique<PublishPlugin>(publishP));
#if __has_include("customplugin.h")
  plugins.push_back(std::make_unique<MyCustomPlugin>(MyCustomPlugin()));
#endif
  addCustomPlugins(plugins);
  for (unsigned int i = 0; i < plugins.size(); i++) {
    plugins[i]->setSystem(this);
    configurator.readConfig(plugins[i].get());

    if (strlen(plugins[i]->getName()) > maxnamelen) {
      maxnamelen = strlen(plugins[i]->getName());
    }
    start(plugins[i].get());
  }
}

void PluginSystem::addCustomPlugins(
    std::vector<std::unique_ptr<Plugin>> &pluginsvec) {
  // add additional plugins here
  // pluginsvec.push_back(std::make_unique<{PluginClassName}>({PluginClassName}()));
}

void PluginSystem::mqttMessageCB(MqttMessage *message) {
  if (mqttPublishCB) {
    auto sender = getPluginById(message->getSenderId());
    if (NULL != sender) {
      char topic[128];
      snprintf(topic, sizeof(topic), "%s/%s", sender->getName(),
               (const char *)message->topic.get());
      if (message->appendTopic) {
        mqttPublishCB(sender->getId(), message->topic.get(),
                      message->payload.get(), message->length, true);
      } else {
        mqttPublishCB(sender->getId(), message->topic.get(),
                      message->payload.get(), message->length, false);
      }
    }
  }
}

void PluginSystem::loop() {
  static uint32_t pluginsloop = 0;

  EVERY_N_SECONDS(1) {
    pluginsloop++;
    if (timercbs.size() > 0) {
      auto it = timercbs.begin();
      while (it != timercbs.end()) {
        if ((pluginsloop % it->interval) == 0) {
          PDebug.printf(PDebugLevel::TRACE, "PluginSystem timercb call: %s\n",
                        it->timername);
          it->timerCb();
          yield();
        }
        if (!it->valid) {
          // erase while iterating should be safe .. according to the internet
          // ;)
          it = timercbs.erase(it);
        } else {
          it++;
        }
      }
    }
    // move new timers
    if (timercbsnew.size() > 0) {
      auto it = timercbs.end();
      timercbs.splice(it, timercbsnew);
    }
  }
  publishInternal();
  for (unsigned int i = 0; i < plugins.size(); i++) {
    if (plugins[i]->isEnabled()) {
      plugins[i]->loop();
      yield();
    }
  }
}

void PluginSystem::start(Plugin *p) {
  if (p->isEnabled()) {
    p->setup();
    p->onTickerSetup();
    p->onMqttSubscribe();
  }
}

void PluginSystem::subscribeMqtt(Plugin *plugin, char *topic, bool append) {
  if (mqttSubscribeCB) {
    mqttSubscribeCB(plugin->getId(), topic, append);
  }
}

void PluginSystem::receiveMqtt(Plugin *plugin, const char *topic,
                               const uint8_t *data, size_t len) {
  MqttMessage m(0, plugin->getId());
  m.setMqtt(topic, data, len);
  // :((
  publisher.publish(std::make_shared<MqttMessage>(m));
}

void PluginSystem::addTimerCb(Plugin *plugin, const char *timername,
                              PLUGIN_TIMER_INTVAL intval, uint32_t interval,
                              std::function<void(void)> timerCb) {
  // PDebug.printf(PDebugLevel::DEBUG,"addTimerCb sender=%d\n",
  // plugin->getId());
  if (timercbs.size() > 0) {
    auto it =
        std::find_if(begin(timercbs), end(timercbs), [timername](auto &e) {
          return ((strcmp(e.timername, timername) == 0) && e.valid);
        });
    if (it != std::end(timercbs)) {
      PDebug.printf(PDebugLevel::WARN,
                    "PluginSystem: addTimerCb(%s): timername exists!\n",
                    timername);
      return;
    }
  }
  if (timercbsnew.size() > 0) {
    auto it = std::find_if(
        begin(timercbsnew), end(timercbsnew),
        [timername](auto &e) { return (strcmp(e.timername, timername) == 0); });
    if (it != std::end(timercbsnew)) {
      PDebug.printf(PDebugLevel::WARN,
                    "PluginSystem: addTimerCb(%s): timername exists!\n",
                    timername);
      return;
    }
  }
  timerentry entry;
  entry.timername = timername;
  uint32_t timerintval = interval;
  if (intval == MINUTE) {
    timerintval *= 60;
  }
  entry.interval = timerintval;
  entry.timerCb = timerCb;
  entry.valid = true;
  timercbsnew.push_back(entry);
  PDebug.printf(PDebugLevel::INFO, "PluginSystem: addTimerCb(%s)\n", timername);
}

void PluginSystem::removeTimerCb(Plugin *plugin, const char *timername) {
  for (auto &entry : timercbs) {
    if (strcmp(entry.timername, timername) == 0) {
      PDebug.printf(PDebugLevel::INFO, "PluginSystem: removeTimerCb (%s)\n",
                    timername);
      entry.valid = false;
      break;
    }
  }
}

bool PluginSystem::hasTimerCb(Plugin *plugin, const char *timername) {
  for (auto &entry : timercbs) {
    if (strcmp(entry.timername, timername) == 0) {
      return true;
    }
  }
  return false;
}

Plugin *PluginSystem::getPluginByIndex(int pluginindex) {

  if (pluginindex >= 0 && pluginindex < plugins.size()) {
    return plugins[pluginindex].get();
  }
  return NULL;
}

Plugin *PluginSystem::getPluginById(int pluginid) {
  for (unsigned int i = 0; i < plugins.size(); i++) {
    if (plugins[i]->getId() == pluginid) {
      return plugins[i].get();
    }
  }
  return NULL;
}

Plugin *PluginSystem::getPluginByName(const char *pluginname) {
  for (unsigned int i = 0; i < plugins.size(); i++) {
    if (strcmp(plugins[i]->getName(), pluginname) == 0) {
      return plugins[i].get();
    }
  }
  return NULL;
}

int PluginSystem::getPluginCount() { return plugins.size(); }

void PluginSystem::publishInternal() { publisher.loop(); }

PluginMessagePublisher &PluginSystem::getPublisher() { return publisher; }
