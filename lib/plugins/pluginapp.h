#ifndef __PLUGINAPP_H__
#define __PLUGINAPP_H__

#include "app.h"
#include "plugin.h"
#include "web/html/h/thirdparty_html.h"

class pluginapp : public app, public System, public settingsCb, public mqttCb, public restCb, public inverterCb
{
public:
    pluginapp() : app() {}
    ~pluginapp() {}
    void setupSettings(settings* appSettings) {
        mSettings = appSettings;
        mSettings->mSettingsCb = this;
    }
    void setupCB(PubMqttType *mqtt, WebType *webtype, RestApiType *restapi)
    {
        DPRINTLN(DBG_INFO, F("setupCB: "));
        mMqtt = mqtt;
        mRestapi = restapi;
        mqtt->mMqttCb = this;
        webtype->getWebSrvPtr()->on("/thirdparty", HTTP_ANY, std::bind(&pluginapp::onHttp, this, std::placeholders::_1));
        webtype->getWebSrvPtr()->on("/thirdpartysetup", HTTP_GET,std::bind(&pluginapp::onThirdpartySetup, this, std::placeholders::_1));
        webtype->getWebSrvPtr()->addHandler(new AsyncCallbackJsonWebHandler("/thirdpartysave", [this](AsyncWebServerRequest *request, JsonVariant &json) {
            JsonObject jsonObj = json.as<JsonObject>();
            loadThirdpartySettings(jsonObj[F("thirdparty")]);
            triggerSave();
            // :)
            request->send(200,F("application/json"),F("{}"));
        }));
        webtype->getWebSrvPtr()->addHandler(new AsyncCallbackJsonWebHandler("/thirdpartyplugins", [this](AsyncWebServerRequest *request, JsonVariant &json) {
            JsonObject requestObj = json.as<JsonObject>();
            AsyncJsonResponse * response = new AsyncJsonResponse();
            JsonObject responseObj = response->getRoot();
            responseObj[F("ok")]=onThirdpartyPlugin(requestObj,responseObj);
            response->setLength();
            request->send(response);
        }));
        restapi->mRestCb = this;
        Inverter<> *iv;
        for (uint8_t i = 0; i < mSys.getNumInverters(); i++)
        {
            iv = mSys.getInverterByPos(i, false);
            if (NULL != iv)
            {
                iv->mInverterCb = this;
            }
        }
    }
    void setup()
    {
        for (unsigned int i = 0; i < plugins.size(); i++)
        {
            plugins[i]->setSystem(this);
            plugins[i]->setup();
            if (strlen(plugins[i]->name) > maxnamelen)
            {
                maxnamelen = strlen(plugins[i]->name);
            }
        }
        app::setup();
    }
    void loop()
    {
        app::loop();
        publishInternal();
        for (unsigned int i = 0; i < plugins.size(); i++)
        {
            if(plugins[i]->isEnabled())
                plugins[i]->loop();
        }
        publish();
        if(saveTpSettings) {
            saveTpSettings = false;
            mSettings->saveSettings();
        }
    }
    void onTickerSetup() {
        for (unsigned int i = 0; i < plugins.size(); i++)
        {
            if(plugins[i]->isEnabled())
                plugins[i]->onTickerSetup();
        }
    }
    bool onThirdpartyPlugin(JsonObject request, JsonObject response) {
        if(request.containsKey(F("pluginid"))) {
            int id = request[F("pluginid")];
            for (unsigned int i = 0; i < plugins.size(); i++)
            {
                if(plugins[i]->getId()==id) {
                    return plugins[i]->onRequest(request,response);
                }
            }
        } else if(request.containsKey(F("pluginname"))) {
            String name = request[F("pluginname")].as<String>();
            for (unsigned int i = 0; i < plugins.size(); i++)
            {
                if(strcmp(name.c_str(),plugins[i]->name)==0) {
                    return plugins[i]->onRequest(request,response);
                }
            }
        }
        return false;
    }

    void onThirdpartySetup(AsyncWebServerRequest *request) {
            AsyncJsonResponse* response = new AsyncJsonResponse(false, 4096);
            JsonObject root = response->getRoot();
            onSaveSettings(root);
            response->setLength();
            request->send(response);
    }

    void onInverterValue(uint8_t inverterId, uint8_t fieldId, float value)
    {
        DPRINTLN(DBG_INFO, F("onInverterValue"));
        InverterMessage message;
        message.inverterId = inverterId;
        message.fieldId = fieldId;
        message.value = value;
        for (unsigned int i = 0; i < plugins.size(); i++)
        {
            if(plugins[i]->isEnabled())
                plugins[i]->inverterCallback(&message);
        }
    }
    void ctrlRequest(Plugin *plugin, JsonObject request)
    {
        mRestapi->ctrlRequest(request);
    }
    /**
     * enqueue a mqtt message in send queue
     * @param topic - mqtt topic
     * @param message - mqtt payload
     * @param appendTopic - append topic to ahoi prefix (inverter/)
     * @return true, if message was enqueued, false otherwise
     */
    bool enqueueMessage(Plugin *plugin, char *topic, char *data, bool append)
    {
        size_t topiclen = strlen(topic) + 1;
        size_t datalen = strlen(data) + 1;
        if (bufferindex + topiclen + datalen > THIRDPARTY_MSG_BUFFERSIZE)
        {
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
        entry.pluginid = plugin->getId();
        q.push(entry);
        return true;
    }
    void subscribeMqtt(Plugin *plugin, char *topic, bool append)
    {
        if (append)
        {
            mMqtt->subscribe(topic);
        }
        else
        {
            mMqtt->subscribeThirdparty(topic);
        }
    }
    void publish()
    {
        // we dont care about real topic length, one size fit's all ;)
        char topic[128];
        while (!q.empty() && mMqtt->isConnected())
        {
            qentry entry = q.front();
            auto sender = getPluginById(entry.pluginid);
            if (NULL != sender)
            {
                if(entry.appendtopic) {
                    snprintf(topic, sizeof(topic), "%s/%s", sender->name, (const char *)buffer + entry.topicindex);
                } else {
                   snprintf(topic, sizeof(topic), "%s", (const char *)buffer + entry.topicindex);
                }
                mMqtt->publish(topic, (const char *)buffer + entry.dataindex, false, entry.appendtopic);
            }
            q.pop();
        }

        bufferindex = 0;
    }

    void addTimerCb(Plugin *plugin, const char* timername, PLUGIN_TIMER_INTVAL intvaltype, uint32_t interval, std::function<void(void)> timerCb)
    {
        if (intvaltype == PLUGIN_TIMER_INTVAL::MINUTE)
        {
            every(timerCb, (interval * 60), timername);
        }
        else if (intvaltype == PLUGIN_TIMER_INTVAL::SECOND)
        {
            every(timerCb, interval, timername);
        }
    }
    void publishMessage(Plugin *sender, PluginMessage& _message) {
        msgs.push(std::make_shared<PluginMessage>(_message)); 
    }
    virtual Plugin *getPluginById(int pluginid) 
    {
        for (unsigned int i = 0; i < plugins.size(); i++)
        {
            if (plugins[i]->getId() == pluginid)
            {
                return plugins[i];
            }
        }
        return NULL;
    }

    virtual Plugin *getPluginByName(const char *pluginname)
    {
        for (unsigned int i = 0; i < plugins.size(); i++)
        {
            if (strcmp(plugins[i]->name, pluginname) == 0)
            {
                return plugins[i];
            }
        }
        return NULL;
    }
    virtual int getPluginCount()
    {
        return plugins.size();
    }
    void addPlugin(Plugin *p)
    {
        plugins.push_back(p);
    }

    void onLoadSettings(DynamicJsonDocument settings)
    {
        DPRINTLN(DBG_INFO, F("onSettingsAction: load settings"));
        if (settings.containsKey(F("thirdparty")))
        {
            loadThirdpartySettings(settings[F("thirdparty")]);
        }
    }

    void loadThirdpartySettings(JsonObject tpsettings) {
            DPRINTLN(DBG_INFO, F("loadThirdpartySettings"));
            for (unsigned int i = 0; i < plugins.size(); i++)
            {
                if (tpsettings.containsKey(plugins[i]->name))
                {
                    plugins[i]->loadSettings(tpsettings[plugins[i]->name]);
                    plugins[i]->loadPluginSettings(tpsettings[plugins[i]->name]);
                }
            }
    }

    void triggerSave() {
        saveTpSettings = true;
    }

    void onGetSetup(JsonObject settings) {
        onSaveSettings(settings);
    }

    void onSaveSettings(JsonObject settings)
    {
        DPRINTLN(DBG_INFO, F("onSettingsAction: save settings"));
        JsonObject tpsettings = settings.createNestedObject(F("thirdparty"));
        for (unsigned int i = 0; i < plugins.size(); i++)
        {
            JsonObject pluginjson = tpsettings.createNestedObject(plugins[i]->name);
            //pluginjson[F("id")] = plugins[i]->getId();
            plugins[i]->saveSettings(pluginjson);
            plugins[i]->savePluginSettings(pluginjson);
        }
    }

    void onMqttConnect()
    {
        mMqtt->subscribe("thirdparty/#");
        for (unsigned int i = 0; i < plugins.size(); i++)
        {
            if(plugins[i]->isEnabled())
                plugins[i]->onMqttSubscribe();
        }
    }
    void onMqttMessage(const char *topic, const uint8_t *payload, size_t len)
    {
        DPRINTLN(DBG_INFO, F("onMessage: ") + String(topic));
        MqttMessage msg;
        msg.topic = (char *)topic;
        msg.payload = (uint8_t *)payload;
        msg.length = len;
        for (unsigned int i = 0; i < plugins.size(); i++)
        {
            if(plugins[i]->isEnabled())
                plugins[i]->mqttCallback(&msg);
        }
    }
    void onRestMenu(JsonObject obj, uint8_t index)
    {
        DPRINTLN(DBG_INFO, F("onMenu"));
        obj[F("name")][index] = "Thirdparty";
        obj[F("link")][index++] = "/thirdparty";
    }

private:
    void onHttp(AsyncWebServerRequest *request)
    {
        DPRINTLN(DBG_INFO, F("onHttp"));
        AsyncWebServerResponse *response = request->beginResponse_P(200, F("text/html"), thirdparty_html, thirdparty_html_len);
        response->addHeader(F("Content-Encoding"), "gzip");
        request->send(response);
    }

    void publishToReceiver(PluginMessage* mes) {
        Plugin* p = getPluginById(mes->getReceiverId());
        if(NULL != p && p->isEnabled()) {
            p->internalDataCallback(mes);
        }
    }

    void publishToAll(PluginMessage* message) {
        int pluginid = message->getSenderId();
        for (unsigned int i = 0; i < plugins.size(); i++)
        {
            if (plugins[i]->getId() != pluginid)
            {
                if(plugins[i]->isEnabled())
                    plugins[i]->internalDataCallback(message);
            }
        }
    }

    void publishInternal()
    {
        while(!msgs.empty()) {
            auto message = msgs.front();
            if(message->hasData()) {
                if(message->getReceiverId()!=0) {
                    publishToReceiver(message.get());
                } else {
                    publishToAll(message.get());
                }
            } else {
                int pluginid = message.get()->getSenderId();
                for (unsigned int i = 0; i < plugins.size(); i++)
                {
                    if (plugins[i]->getId() != pluginid)
                    {
                        if(plugins[i]->isEnabled())
                            plugins[i]->internalCallback(message);
                    }
                }
            }
            msgs.pop();
            // do i need this? :/
            message.reset();
        }
    }

    PubMqttType *mMqtt;
    RestApiType *mRestapi;
    settings *mSettings;
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
    std::vector<Plugin *> plugins;
    std::queue<std::shared_ptr<PluginMessage>> msgs;
};

#endif /*__PLUGINAPP_H__*/
