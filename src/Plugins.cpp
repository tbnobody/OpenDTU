
#include "Plugins.h"
#include "MessageOutput.h"
#include "MqttSettings.h"
#include "string.h"

#include "HoymilesPlugin.h"
#include "InverterPlugin.h"
#include "MeterPlugin.h"
#include "PluginConfiguration.h"
#include "PowercontrolPlugin.h"
#include "PublishPlugin.h"
#include "develsupportplugin.h"
#if __has_include("customplugin.h")
#include "customplugin.h"
#endif

PluginsClass Plugins;

void PluginsClass::loop() {
  static uint32_t pluginsloop = 0;

  EVERY_N_SECONDS(1) {
    pluginsloop++;
    for (uint32_t i = 0; i < timercbs.size(); i++) {
      if ((pluginsloop % timercbs[i].interval) == 0) {
        // PDebug.printf(PDebugLevel::DEBUG,"PluginsClass timercb call: %s\n",
        // timercbs[i].timername);
        timercbs[i].timerCb();
        yield();
      }
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

void PluginsClass::start(Plugin *p) {
  if (p->isEnabled()) {
    p->setup();
    p->onTickerSetup();
    p->onMqttSubscribe();
  }
}

void PluginsClass::subscribeMqtt(Plugin *plugin, char *topic, bool append) {
  //   PDebug.printf(PDebugLevel::DEBUG,"PluginsClass::subscribeMqtt %s: %s\n",
  //   plugin->name, topic);
  MqttSettings.subscribe(
      topic, 0,
      [&, plugin](const espMqttClientTypes::MessageProperties &properties,
                  const char *topic, const uint8_t *payload, size_t len,
                  size_t index, size_t total) {
        //       PDebug.printf(PDebugLevel::DEBUG,"PluginsClass::mqttCb
        //       topic=%s\n", topic);
        MqttMessage m(0, plugin->getId());
        m.setMqtt(topic, payload, len);
        // :((
        publisher.publish(std::make_shared<MqttMessage>(m));
      });
}

void PluginsClass::addTimerCb(Plugin *plugin, const char *timername,
                              PLUGIN_TIMER_INTVAL intval, uint32_t interval,
                              std::function<void(void)> timerCb) {
  // PDebug.printf(PDebugLevel::DEBUG,"PluginsClass::addTimerCb sender=%d\n",
  // plugin->getId());
  timerentry entry;
  entry.timername = timername;
  uint32_t timerintval = interval;
  if (intval == MINUTE) {
    timerintval *= 60;
  }
  entry.interval = timerintval;
  entry.timerCb = timerCb;
  timercbs.push_back(entry);
}

void PluginsClass::mqttMessageCB(MqttMessage *message) {
  // we dont care about real topic length, one size fit's all ;)
  //   char topic[128];
  if (!MqttSettings.getConnected()) {
    PDebug.printf(PDebugLevel::DEBUG,
                  "PluginsClass: mqtt not connected. can not send message!");
    return;
  }
  PDebug.printf(PDebugLevel::DEBUG, "PluginsClass: publish mqtt nmessage!");
  auto sender = getPluginById(message->getSenderId());
  if (NULL != sender) {
    char topic[128];
    snprintf(topic, sizeof(topic), "%s/%s", sender->getName(),
             (const char *)message->topic.get());
    if (message->appendTopic) {
      MqttSettings.publish(topic, (const char *)message->payloadToChar().get());
    } else {
      MqttSettings.publishGeneric(
          topic, (const char *)message->payloadToChar().get(), false, 0);
    }
  }
}

Plugin *PluginsClass::getPluginByIndex(int pluginindex) {

  if (pluginindex >= 0 && pluginindex < plugins.size()) {
    return plugins[pluginindex].get();
  }
  return NULL;
}

Plugin *PluginsClass::getPluginById(int pluginid) {
  for (unsigned int i = 0; i < plugins.size(); i++) {
    if (plugins[i]->getId() == pluginid) {
      return plugins[i].get();
    }
  }
  return NULL;
}

Plugin *PluginsClass::getPluginByName(const char *pluginname) {
  for (unsigned int i = 0; i < plugins.size(); i++) {
    if (strcmp(plugins[i]->getName(), pluginname) == 0) {
      return plugins[i].get();
    }
  }
  return NULL;
}

int PluginsClass::getPluginCount() { return plugins.size(); }

void PluginsClass::addPlugin(std::unique_ptr<Plugin> &p) {
  plugins.push_back(std::move(p));
}

void PluginsClass::publishInternal() {
#ifndef ESPSINGLECORE

#else
  publisher.loop();
#endif
}

PluginMessagePublisher &PluginsClass::getPublisher() { return publisher; }

#ifndef ESPSINGLECORE
extern "C" {
void pluginTaskFunc(void *parameter);
}

void pluginTaskFunc(void *parameter) {
  for (;;) {
    Plugins.getPublisher().loop();
  }
}
#endif

void PluginsClass::addCustomPlugins() {
  // add additional plugins here
  // plugins.push_back(std::make_unique<{PluginClassName}>({PluginClassName}()));
}

void PluginsClass::init() {
  MessageOutput.setLevel(MessageOutputDebugLevel::DEBUG_INFO);
  PDebug.setPrint(&MessageOutput);
  plugins.push_back(std::make_unique<DevelSupportPlugin>(DevelSupportPlugin()));
  plugins.push_back(std::make_unique<HoymilesPlugin>(HoymilesPlugin()));
  plugins.push_back(std::make_unique<MeterPlugin>(MeterPlugin()));
  plugins.push_back(std::make_unique<InverterPlugin>(InverterPlugin()));
  plugins.push_back(std::make_unique<PowercontrolPlugin>(PowercontrolPlugin()));
  PublishPlugin publishP = PublishPlugin();
  publishP.mqttMessageCB(
      std::bind(&PluginsClass::mqttMessageCB, this, std::placeholders::_1));
  plugins.push_back(std::make_unique<PublishPlugin>(publishP));
#if __has_include("customplugin.h")
  plugins.push_back(std::make_unique<MyCustomPlugin>(MyCustomPlugin()));
#endif
  addCustomPlugins();

  for (unsigned int i = 0; i < plugins.size(); i++) {
    plugins[i]->setSystem(this);
    PluginConfiguration.read(plugins[i].get());
    if (strlen(plugins[i]->getName()) > maxnamelen) {
      maxnamelen = strlen(plugins[i]->getName());
    }
    start(plugins[i].get());
  }
#ifndef ESPSINGLECORE
  xTaskCreatePinnedToCore(pluginTaskFunc, /* Function to implement the task */
                          "pluginTask",   /* Name of the task */
                          10000,          /* Stack size in words */
                          NULL,           /* Task input parameter */
                          0,              /* Priority of the task */
                          &pluginTask,    /* Task handle. */
                          0);             /* Core where the task should run */
#endif
  PDebug.printf(PDebugLevel::DEBUG, "PluginsClass::init\n");
}
