// hm350 112 182 842 985
#include "Plugins.h"
#include "MessageOutput.h"
#include "MqttSettings.h"
#include "string.h"

#include "InverterPlugin.h"
#include "MeterPlugin.h"
#include "PluginConfiguration.h"
#include "PowercontrolPlugin.h"
#include "demoplugin.h"

PluginsClass Plugins;
demoPlugin demoP = demoPlugin();
MeterPlugin meterP = MeterPlugin();
InverterPlugin inverterP = InverterPlugin();
PowercontrolPlugin powercontrollerP = PowercontrolPlugin();

void PluginsClass::init()
{
    addPlugin(&demoP);
    addPlugin(&meterP);
    addPlugin(&inverterP);
    addPlugin(&powercontrollerP);
    for (unsigned int i = 0; i < plugins.size(); i++) {
        plugins[i]->setSystem(this);
        PluginConfiguration.read(plugins[i]);
        if (strlen(plugins[i]->name) > maxnamelen) {
            maxnamelen = strlen(plugins[i]->name);
        }
        if (plugins[i]->isEnabled()) {
            plugins[i]->setup();
            plugins[i]->onTickerSetup();
            plugins[i]->onMqttSubscribe();
        }
    }
    MessageOutput.println("PluginsClass::init");
}

void PluginsClass::loop()
{
    static uint32_t pluginsloop = 0;
    EVERY_N_SECONDS(5)
    {
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
            // MessageOutput.printf("PluginsClass::loop plugin(name=%s,id=%d,enabled=%d)\n", plugins[i]->name, plugins[i]->getId(), plugins[i]->isEnabled());
        }
    }
    EVERY_N_SECONDS(1)
    {
        pluginsloop++;
        for (uint32_t i = 0; i < timercbs.size(); i++) {
            if ((pluginsloop % timercbs[i].interval) == 0) {
                // MessageOutput.printf("PluginsClass timercb call: %s\n", timercbs[i].timername);
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
    publish();
}

void PluginsClass::subscribeMqtt(Plugin* plugin, char* topic, bool append)
{
    //   MessageOutput.printf("PluginsClass::subscribeMqtt %s: %s\n", plugin->name, topic);
    MqttSettings.subscribe(topic, 0, [plugin](const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total) {
        //       MessageOutput.printf("PluginsClass::mqttCb topic=%s\n", topic);
        MqttMessage m;
        m.topic = topic;
        m.payload = payload;
        m.length = len;
        plugin->mqttCallback(&m);
    });
}

bool PluginsClass::enqueueMessage(Plugin* sender, char* topic, char* data, bool append)
{
    // MessageOutput.printf("PluginsClass::enqueueMessage sender=%d\n", sender->getId());
    size_t topiclen = strlen(topic) + 1;
    size_t datalen = strlen(data) + 1;
    if (bufferindex + topiclen + datalen > THIRDPARTY_MSG_BUFFERSIZE) {
        return false;
    }
    qentry entry;
    entry.topicindex = bufferindex;
    memcpy(buffer + bufferindex, topic, topiclen);
    bufferindex += topiclen;
    entry.dataindex = bufferindex;
    memcpy(buffer + bufferindex, data, datalen);
    bufferindex += datalen;
    entry.appendtopic = append;
    entry.pluginid = sender->getId();
    q.push(entry);
    return true;
}
void PluginsClass::publishMessage(Plugin* sender, PluginMessage& message)
{
    // MessageOutput.printf("plugins::publishMessage sender=%d receiver=%d\n", sender->getId(),message.getReceiverId());
    msgs.push(std::make_shared<PluginMessage>(message));
}
void PluginsClass::addTimerCb(Plugin* plugin, const char* timername, PLUGIN_TIMER_INTVAL intval, uint32_t interval, std::function<void(void)> timerCb)
{
    // MessageOutput.printf("PluginsClass::addTimerCb sender=%d\n", plugin->getId());
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
void PluginsClass::publish()
{
    // we dont care about real topic length, one size fit's all ;)
    char topic[128];
    while (!q.empty() && MqttSettings.getConnected()) {
        qentry entry = q.front();
        auto sender = getPluginById(entry.pluginid);
        if (NULL != sender) {
            snprintf(topic, sizeof(topic), "%s/%s", sender->name, (const char*)buffer + entry.topicindex);
            if (entry.appendtopic) {
                MqttSettings.publish(topic, (const char*)buffer + entry.dataindex);
            } else {
                MqttSettings.publishGeneric(topic, (const char*)buffer + entry.dataindex, false, 0);
            }
        }
        q.pop();
    }

    bufferindex = 0;
}

Plugin* PluginsClass::getPluginByIndex(int pluginindex)
{

    if (pluginindex >= 0 && pluginindex < plugins.size()) {
        return plugins[pluginindex];
    }
    return NULL;
}

Plugin* PluginsClass::getPluginById(int pluginid)
{
    for (unsigned int i = 0; i < plugins.size(); i++) {
        if (plugins[i]->getId() == pluginid) {
            return plugins[i];
        }
    }
    return NULL;
}

Plugin* PluginsClass::getPluginByName(const char* pluginname)
{
    for (unsigned int i = 0; i < plugins.size(); i++) {
        if (strcmp(plugins[i]->name, pluginname) == 0) {
            return plugins[i];
        }
    }
    return NULL;
}

int PluginsClass::getPluginCount()
{
    return plugins.size();
}

void PluginsClass::addPlugin(Plugin* p)
{
    plugins.push_back(p);
}

void PluginsClass::publishToReceiver(PluginMessage* mes)
{
    Plugin* p = getPluginById(mes->getReceiverId());
    if (NULL != p && p->isEnabled()) {
        p->internalDataCallback(mes);
    }
}

void PluginsClass::publishToAll(PluginMessage* message)
{
    //   MessageOutput.printf("plugins publishToAll sender=%d\n",message->getSenderId());
    int pluginid = message->getSenderId();
    for (unsigned int i = 0; i < plugins.size(); i++) {
        if (plugins[i]->getId() != pluginid) {
            if (plugins[i]->isEnabled()) {
                //                MessageOutput.printf("plugins msg sender=%d to plugin=%d\n",message->getSenderId(),plugins[i]->getId());
                plugins[i]->internalDataCallback(message);
            }
        }
    }
}

void PluginsClass::publishInternal()
{
    while (!msgs.empty()) {
        auto message = msgs.front();
        if (message->hasData()) {
            // MessageOutput.printf("plugins publishInternal sender=%d\n",message->getSenderId());
            if (message->getReceiverId() != 0) {
                publishToReceiver(message.get());
            } else {
                publishToAll(message.get());
            }
        } else {
            int pluginid = message.get()->getSenderId();
            for (unsigned int i = 0; i < plugins.size(); i++) {
                if (plugins[i]->getId() != pluginid) {
                    if (plugins[i]->isEnabled())
                        plugins[i]->internalCallback(message);
                }
            }
        }
        msgs.pop();
        // do i need this? :/
        message.reset();
    }
}
