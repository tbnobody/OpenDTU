#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include "generic.hpp"
#if __has_include("pluginids.h")
    #include "pluginids.h"
#endif
#include <string>
#include <functional>
#include <ArduinoJson.h>
#include "MessageOutput.h"
#include "Hoymiles.h"

enum TYPEIDS { 
    DUMMY, 
    FLOATVALUE_TYPE,INTVALUE_TYPE,LONGVALUE_TYPE,BOOLVALUE_TYPE,STRINGVALUE_TYPE,
    PLUGINMESSAGE_TYPE
};

typedef Data<float,FLOATVALUE_TYPE> FloatValue;
template <>
struct EntityIds<FloatValue>
{
    enum { type_id = TYPEIDS::FLOATVALUE_TYPE };
};
typedef Data<bool,BOOLVALUE_TYPE> BoolValue;
template <>
struct EntityIds<BoolValue>
{
    enum { type_id = TYPEIDS::BOOLVALUE_TYPE };
};
typedef Data<int,INTVALUE_TYPE> IntValue;
template <>
struct EntityIds<IntValue>
{
    enum { type_id = TYPEIDS::INTVALUE_TYPE };
};
typedef Data<uint64_t,LONGVALUE_TYPE> LongValue;
template <>
struct EntityIds<LongValue>
{
    enum { type_id = TYPEIDS::LONGVALUE_TYPE };
};
typedef Data<String,STRINGVALUE_TYPE> StringValue;
template <>
struct EntityIds<StringValue>
{
    enum { type_id = TYPEIDS::STRINGVALUE_TYPE };
};
class PluginMessage;
template <>
struct EntityIds<PluginMessage>
{
    enum { type_id = TYPEIDS::PLUGINMESSAGE_TYPE };
};

//class Message;
enum class METADATA_TAGS { EMPTY_TAG, SENDERID, RECEIVERID, MSGTS, TESTTAG};
class MetaData : public ContainerMap<METADATA_TAGS,Entity> {

    public:
    int getSenderId() {return getValueAs<IntValue>(METADATA_TAGS::SENDERID).value;}
    int getReceiverId() {return getValueAs<IntValue>(METADATA_TAGS::RECEIVERID).value;}
    template <typename T>
    T getTagAs(METADATA_TAGS id) {
        return getValueAs<T>(id);
    }
    bool hasTag(METADATA_TAGS id) {
        return hasKey(id);
    }
    protected:
    void setSenderId(int id) { add(METADATA_TAGS::SENDERID,IntValue(0,id)); }
    void setReceiverId(int id) { add(METADATA_TAGS::RECEIVERID,IntValue(0,id)); }
    template <typename T>
    void addTag(METADATA_TAGS id, T tag) {
        add(id,tag);
    }

    friend class PluginMessage;
};

class InverterMessage
{
public:
     /**
     * invererSerial - id of inverter 
     */
    uint64_t inverterSerial;
    String inverterStringSerial;
    
    ChannelNum_t channelNumber;
    ChannelType_t channelType;
    /**
     *  fieldId - see hmDefines.h => field types
     */
    FieldId_t fieldId;
    /**
    *value - value transmited by inverter
    */
    float value;
};

class MqttMessage
{
public:
    const char *topic;
    const uint8_t *payload;
    unsigned int length;
    bool appendTopic = true;
};

typedef enum
{
    SECOND,
    MINUTE
} PLUGIN_TIMER_INTVAL;
class Plugin;
class PluginMessage;

class System
{
public:
    virtual void subscribeMqtt(Plugin *plugin, char *topic, bool append) = 0;
    virtual bool enqueueMessage(Plugin *sender, char *topic, char *data, bool append) = 0;
    virtual void publishMessage(Plugin *sender, PluginMessage& message) = 0;
    virtual void addTimerCb(Plugin *plugin, const char* timername, PLUGIN_TIMER_INTVAL intval, uint32_t interval, std::function<void(void)> timerCb) = 0;
    virtual Plugin *getPluginById(int pluginid);
    virtual Plugin *getPluginByName(const char *pluginname);
    virtual int getPluginCount();

};

/**
 * Plugin interface
 */
class Plugin
{
public:
    Plugin(int _id, const char *_name)
    {
        id = _id;
        name = _name;
    }
    int getId() { return id; }
    bool isEnabled() { return enabled; }
    void loadPluginSettings(JsonObject s) {
        if(s.containsKey(F("enabled"))) {
            enabled = s[F("enabled")];
        }
    }
    void savePluginSettings(JsonObject s) {
        s[F("enabled")]=enabled;
    }
    void setSystem(System *s)
    {
        system = s;
    }
    /**
     * setup
     *
     * called at end of ahoi main setup
     *
     * @param app - pointer to ahoi settings
     */
    virtual void setup() {}
    /**
     * loop
     *
     * called at end of ahoi main loop
     *
     * @param app - pointer to ahoi app
     */
    virtual void loop() {} 
    /**
     * inverterCallback
     *
     * will be called at end of Inverter.addValue(...)
     *
     *  @param InverterMessage
     */
    virtual void inverterCallback(const InverterMessage *message) {}
    /**
     * mqttCallback
     *
     * will be called at end of app::cbMqtt
     *
     *  @param MqttMessage
     */
    virtual void mqttCallback(const MqttMessage *message) {}
        /**
     * internalCallback
     *
     * will be called from 'system'
     *
     *  @param PluginMessage
     */
    virtual void internalDataCallback(PluginMessage *message) = 0;
    /**
     * internalCallback
     *
     * will be called from 'system'
     *
     *  @param PluginMessage
     */
    virtual void internalCallback(std::shared_ptr<PluginMessage> message) {}
    /**
     * called when json message was posted to /thirdpartyplugins.
     * message must contain either 'pluginid' or 'pluginname'.
     * 
     * e.g.
     * 
     * {"pluginname":"someplugin","paramname":"paramvalue"}
     * 
     *  @param request - JsonObject
     *  @param respone - JsonObject
     *  @return true if request was handled - false otherwise
     */
    virtual bool onRequest(JsonObject request, JsonObject response) { return false; }
    /**
     * called when mqtt was connected/reconnected 
     * subscribe your topics here! :)
     */
    virtual void onMqttSubscribe() {}
    /**
     * called when tickers should be setup
     */
    virtual void onTickerSetup() {}
    /**
     * @brief called when settings are loaded
     * 
     * @param settings - jsonobject with plugin config
     */
    virtual void loadSettings(JsonObject settings) {}
    /**
     * called when settings are saved
     * 
     * @param settings - jsonobject for plugin config
     */
    virtual void saveSettings(JsonObject settings) {}
    /**
     * subscribe mqtt topic. 
     * use: onMqttSubscribe()
     * 
     * @param topic
     * @param append - true if topic should be appended to DEFAULT TOPIC PREFIX - false otherwise
     */
    void subscribeMqtt(char *topic, bool append)
    {
        if (system)
        {
            system->subscribeMqtt(this, topic, append);
        }
    }
    /**
     * @brief enqueue mqtt message
     * 
     * @param topic - mqtt topic
     * @param data - data to be send
     * @param append - true: {DEFAULT MQTT TOPIC}/{pluginname}/{topic}, false: {topic}
     * @return true if message was queued successful - false otherwise 
     */
    bool enqueueMessage(char *topic, char *data, bool append = true)
    {
        if (system)
        {
            return system->enqueueMessage(this, topic, data, append);
        }
        return false;
    }
    // void publishInternalValues(IdEntity...  &elements) {
    //     if (system)
    //     {
    //         system->publishInternalValues(this,elements);
    //     }
    // }
    /**
     * @brief publish internal message to all plugins
     * 
     * @param valueid - value identifier
     * @param value
     */
    void publishMessage(PluginMessage& message)
    {
        if (system)
        {
            system->publishMessage(this,message);
        }
    }
    /**
     * @brief add timer callback.
     * 
     * @param intvaltype - MINUTE / SECOND
     * @param interval
     * @param timerCb - callback function
     * @param timername
     */
    void addTimerCb(PLUGIN_TIMER_INTVAL intvaltype, uint32_t interval, std::function<void(void)> timerCb, const char* timername)
    {
        if (system)
        {
            system->addTimerCb(this, timername, intvaltype, interval, timerCb);
        }
    }
    const char *name;

private:
    int id;
    System *system;
    bool enabled = false;
};

class EntityError {};
class PluginMessage :  public ContainerVector<Entity>, public Entity {

    public:
    PluginMessage(Plugin &plugin) : PluginMessage(plugin.getId(),0) {}
    PluginMessage(int senderid, int receiverid) : Entity(TYPEIDS::PLUGINMESSAGE_TYPE) {
        headers.setSenderId(senderid);
        headers.setReceiverId(receiverid);
        headers.add(METADATA_TAGS::MSGTS,LongValue(0,millis()));
    }
    PluginMessage(const PluginMessage &v) = default;
    //PluginMessage(int senderid) : PluginMessage(senderid,0) { }
    MetaData& getMetaData() { return headers; }
    bool hasData() { return (entities.size()>0);}
    bool from(int senderid) {
        return (headers.getSenderId()==senderid);
    }
    int getSenderId() { return headers.getSenderId(); }
    int getReceiverId() { return headers.getReceiverId(); }
    bool has(int senderid, int dataid) {
        return (from(senderid)&&hasDataId(dataid));
    }
    bool hasDataId(int id) {
        for(unsigned int i=0; i < getEntryCount() ; i++) {
            if(get(i).getId()==id)
                return true;
        }
        return false;
    }
    template <typename T>
    void addTag(METADATA_TAGS tag, T &e) {
        headers.addTag(tag,e);
    }
    template <typename T>
    void addTag(METADATA_TAGS tag, T &&e) {
        headers.addTag(tag,std::move(e));
    }
    template <typename T>
    T getDataAs(int dataid) {
        for(unsigned int i=0; i < getEntryCount() ; i++) {
            if(get(i).getId()==dataid)
                return getAs<T>(i).value;
        }
        throw(EntityError());
    }
    protected:
    MetaData headers;
 };
#endif