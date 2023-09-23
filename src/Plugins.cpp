
#include "Plugins.h"
#include "MessageOutput.h"
#include "MqttSettings.h"
#include "string.h"

#include "InverterPlugin.h"
#include "MeterPlugin.h"
#include "PluginConfiguration.h"
#include "PowercontrolPlugin.h"
#include "demoplugin.h"
#include "PublishPlugin.h"

PluginsClass Plugins;
demoPlugin demoP = demoPlugin();
MeterPlugin meterP = MeterPlugin();
InverterPlugin inverterP = InverterPlugin();
PowercontrolPlugin powercontrollerP = PowercontrolPlugin();
PublishPlugin publishP = PublishPlugin();

void PluginsClass::init() {
  addPlugin(&demoP);
  addPlugin(&meterP);
  addPlugin(&inverterP);
  addPlugin(&powercontrollerP);
  publishP.mqttMessageCB(std::bind(&PluginsClass::mqttMessageCB, this, std::placeholders::_1));
  addPlugin(&publishP);
  for (unsigned int i = 0; i < plugins.size(); i++) {
    plugins[i]->setSystem(this);
    PluginConfiguration.read(plugins[i]);
    if (strlen(plugins[i]->getName()) > maxnamelen) {
      maxnamelen = strlen(plugins[i]->getName());
    }
    start(plugins[i]);
  }
  MessageOutput.println("PluginsClass::init");
}

void PluginsClass::loop() {
  static uint32_t pluginsloop = 0;
  EVERY_N_SECONDS(5) {
    /*
    MessageOutput.printf("PluginsClass::loop plugincount=%d\n", plugins.size());
    //PluginConfiguration.debug();
    Plugin* p = getPluginByName("demo");
    PluginConfiguration.read(p);
     File f = LittleFS.open("/demo", "r", false);
     MessageOutput.printf("** /demo size: %d\n",f.size());
     MessageOutput.println(f.readString());
     f.close();
     */
    for (unsigned int i = 0; i < plugins.size(); i++) {
      // MessageOutput.printf("PluginsClass::loop
      // plugin(name=%s,id=%d,enabled=%d)\n", plugins[i]->name,
      // plugins[i]->getId(), plugins[i]->isEnabled());
    }
  }
  EVERY_N_SECONDS(1) {
    pluginsloop++;
    for (uint32_t i = 0; i < timercbs.size(); i++) {
      if ((pluginsloop % timercbs[i].interval) == 0) {
        // MessageOutput.printf("PluginsClass timercb call: %s\n",
        // timercbs[i].timername);
        timercbs[i].timerCb();
      }
    }
  }
  publishInternal();
  for (unsigned int i = 0; i < plugins.size(); i++) {
    if (plugins[i]->isEnabled()) {
      plugins[i]->loop();
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
  //   MessageOutput.printf("PluginsClass::subscribeMqtt %s: %s\n",
  //   plugin->name, topic);
  MqttSettings.subscribe(
      topic, 0,
      [&,plugin](const espMqttClientTypes::MessageProperties &properties,
               const char *topic, const uint8_t *payload, size_t len,
               size_t index, size_t total) {
        //       MessageOutput.printf("PluginsClass::mqttCb topic=%s\n", topic);
        MqttMessage m(0, plugin->getId());
        m.setMqtt(topic,payload,len);
        msgs.push(std::make_shared<MqttMessage>(m));
      });
}

void PluginsClass::addTimerCb(Plugin *plugin, const char *timername,
                              PLUGIN_TIMER_INTVAL intval, uint32_t interval,
                              std::function<void(void)> timerCb) {
  // MessageOutput.printf("PluginsClass::addTimerCb sender=%d\n",
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

void PluginsClass::mqttMessageCB(MqttMessage* message) {
  // we dont care about real topic length, one size fit's all ;)
//   char topic[128];
   if (!MqttSettings.getConnected()) {
        MessageOutput.printf("PluginsClass: mqtt not connected. can not send message!");
        return;
   }
    MessageOutput.printf("PluginsClass: publish mqtt nmessage!");
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
    return plugins[pluginindex];
  }
  return NULL;
}

Plugin *PluginsClass::getPluginById(int pluginid) {
  for (unsigned int i = 0; i < plugins.size(); i++) {
    if (plugins[i]->getId() == pluginid) {
      return plugins[i];
    }
  }
  return NULL;
}

Plugin *PluginsClass::getPluginByName(const char *pluginname) {
  for (unsigned int i = 0; i < plugins.size(); i++) {
    if (strcmp(plugins[i]->getName(), pluginname) == 0) {
      return plugins[i];
    }
  }
  return NULL;
}

int PluginsClass::getPluginCount() { return plugins.size(); }

void PluginsClass::addPlugin(Plugin *p) { plugins.push_back(p); }

void PluginsClass::publishToReceiver(std::shared_ptr<PluginMessage> mes) {
  Plugin *p = getPluginById(mes->getReceiverId());
  if (NULL != p && p->isEnabled()) {
    p->internalCallback(mes);
  }
}

void PluginsClass::publishToAll(std::shared_ptr<PluginMessage> message) {
  // MessageOutput.printf("plugins publishToAll
  // sender=%d\n",message->getSenderId());
  int pluginid = message->getSenderId();
  for (unsigned int i = 0; i < plugins.size(); i++) {
    if (plugins[i]->getId() != pluginid) {
      if (plugins[i]->isEnabled()) {
        //                MessageOutput.printf("plugins msg sender=%d to
        //                plugin=%d\n",message->getSenderId(),plugins[i]->getId());
        plugins[i]->internalCallback(message);
      }
    }
  }
}

void PluginsClass::publishInternal() {
  while (msgs.size()>0l) {
    auto message = msgs.front();

    DBGPRINTMESSAGEFROMTO(DBG_INFO,"mainloop start",message);
    if (message->getReceiverId() != 0) {
      publishToReceiver(message);
    } else {
      publishToAll(message);
    }
    DBGPRINTMESSAGEDURATION(DBG_INFO,"mainloop end",message);

    msgs.pop();
    // do i need this? :/
    message.reset();
  }
}

PluginMessagePublisher &PluginsClass::getPublisher() { return publisher; }
