#ifndef __PLUGINIDS_H__
#define __PLUGINIDS_H__

#ifndef NDEBUG
enum PluginIds {
    PluginDemo = 999,
    PluginInverter = 1,
    PluginDemo2 = 99,
    PluginMeter = 2,
    PluginPowercontrol = 3,
};
enum PluginDemoIds {
    SOMEVALUE,
    SOMEOTHERVALUE,
};
enum PluginInverterIds {
    ACPOWER_INVERTER,
    ACPOWER_INVERTERSTRING,
    ACPOWER_PRODUCTION,
    ACPOWER_PRODUCTION_TOTAL,
};
enum PluginDemo2Ids {
    SOMEOTHERPLUGINOUTPUT,
};
enum PluginMeterIds {
    METER_SERIAL,
    METER_POWER,
};
enum PluginPowercontrolIds {
    INVERTER,
    INVERTERSTRING,
    POWERLIMIT,
};
const char PluginDemoIdString[] = "Demo";
const char PluginDemoSOMEVALUEIdString[] = "SOMEVALUE";
const char PluginDemoSOMEOTHERVALUEIdString[] = "SOMEOTHERVALUE";
const char PluginInverterIdString[] = "Inverter";
const char PluginInverterACPOWER_INVERTERIdString[] = "ACPOWER_INVERTER";
const char PluginInverterACPOWER_INVERTERSTRINGIdString[] = "ACPOWER_INVERTERSTRING";
const char PluginInverterACPOWER_PRODUCTIONIdString[] = "ACPOWER_PRODUCTION";
const char PluginInverterACPOWER_PRODUCTION_TOTALIdString[] = "ACPOWER_PRODUCTION_TOTAL";
const char PluginDemo2IdString[] = "Demo2";
const char PluginDemo2SOMEOTHERPLUGINOUTPUTIdString[] = "SOMEOTHERPLUGINOUTPUT";
const char PluginMeterIdString[] = "Meter";
const char PluginMeterMETER_SERIALIdString[] = "METER_SERIAL";
const char PluginMeterMETER_POWERIdString[] = "METER_POWER";
const char PluginPowercontrolIdString[] = "Powercontrol";
const char PluginPowercontrolINVERTERIdString[] = "INVERTER";
const char PluginPowercontrolINVERTERSTRINGIdString[] = "INVERTERSTRING";
const char PluginPowercontrolPOWERLIMITIdString[] = "POWERLIMIT";
const char PluginUnknown[] = "unknown";
const char TAG_EMPTY_TAG_String[] = "Empty_tag";
const char TAG_SENDERID_String[] = "Senderid";
const char TAG_RECEIVERID_String[] = "Receiverid";
const char TAG_TESTTAG_String[] = "Testtag";

#endif // NDEBUG
#ifdef NDEBUG
#define DBGPRINTMESSAGETAGSLN(level,message)
#define DBGPRINTMESSAGELN(level,message)
#else
class PluginDebug {
public:
static const char* getTagNameDebug(int id) {
	switch(id){
		case 0: return TAG_EMPTY_TAG_String;
		case 1: return TAG_SENDERID_String;
		case 2: return TAG_RECEIVERID_String;
		case 3: return TAG_TESTTAG_String;
		default: return PluginUnknown;
}
}
static const char* getPluginNameDebug(int pid) {
	switch(pid){
		case 999: return PluginDemoIdString;
		case 1: return PluginInverterIdString;
		case 99: return PluginDemo2IdString;
		case 2: return PluginMeterIdString;
		case 3: return PluginPowercontrolIdString;
		default: return PluginUnknown;
}
}
static const char* getPluginValueNameDebug(int pid, int vid) {
	switch(pid){
		case 999: switch(vid){			
				case 0: return PluginDemoSOMEVALUEIdString;
				case 1: return PluginDemoSOMEOTHERVALUEIdString;
				default: return PluginUnknown;
}
		case 1: switch(vid){			
				case 0: return PluginInverterACPOWER_INVERTERIdString;
				case 1: return PluginInverterACPOWER_INVERTERSTRINGIdString;
				case 2: return PluginInverterACPOWER_PRODUCTIONIdString;
				case 3: return PluginInverterACPOWER_PRODUCTION_TOTALIdString;
				default: return PluginUnknown;
}
		case 99: switch(vid){			
				case 0: return PluginDemo2SOMEOTHERPLUGINOUTPUTIdString;
				default: return PluginUnknown;
}
		case 2: switch(vid){			
				case 0: return PluginMeterMETER_SERIALIdString;
				case 1: return PluginMeterMETER_POWERIdString;
				default: return PluginUnknown;
}
		case 3: switch(vid){			
				case 0: return PluginPowercontrolINVERTERIdString;
				case 1: return PluginPowercontrolINVERTERSTRINGIdString;
				case 2: return PluginPowercontrolPOWERLIMITIdString;
				default: return PluginUnknown;
}
			default: return PluginUnknown;
}
}
}; /* PluginDebug class end */
#define DBGPRINTMESSAGELN(level,prefix,message) ({\
       char msgbuffer[1024];\
         snprintf(msgbuffer,sizeof(msgbuffer),"%s sender %s ",prefix,PluginDebug::getPluginNameDebug(message->getSenderId()));\
       MessageOutput.println(msgbuffer);\
        if(message->hasData()) {\
    for(unsigned int index = 0 ; index < message->getEntryCount(); index++) {\
        if(message->isType<BoolValue>(index))\
            snprintf(msgbuffer,sizeof(msgbuffer),"%s-%s: bool %d",PluginDebug::getPluginNameDebug(message->getSenderId()),PluginDebug::getPluginValueNameDebug(message->getSenderId(),message->get(index).getId()),message->getAs<BoolValue>(index).value);\
        else if(message->isType<FloatValue>(index))\
            snprintf(msgbuffer,sizeof(msgbuffer),"%s-%s: float %f",PluginDebug::getPluginNameDebug(message->getSenderId()),PluginDebug::getPluginValueNameDebug(message->getSenderId(),message->get(index).getId()),message->getAs<FloatValue>(index).value);\
        else if(message->isType<IntValue>(index))\
            snprintf(msgbuffer,sizeof(msgbuffer),"%s-%s: int %d",PluginDebug::getPluginNameDebug(message->getSenderId()),PluginDebug::getPluginValueNameDebug(message->getSenderId(),message->get(index).getId()),message->getAs<IntValue>(index).value);\
        else if(message->isType<StringValue>(index))\
            snprintf(msgbuffer,sizeof(msgbuffer),"%s-%s: string %s",PluginDebug::getPluginNameDebug(message->getSenderId()),PluginDebug::getPluginValueNameDebug(message->getSenderId(),message->get(index).getId()),message->getAs<StringValue>(index).value.c_str());\
        else if(message->isType<LongValue>(index))\
            snprintf(msgbuffer,sizeof(msgbuffer),"%s-%s: long %llu",PluginDebug::getPluginNameDebug(message->getSenderId()),PluginDebug::getPluginValueNameDebug(message->getSenderId(),message->get(index).getId()),message->getAs<LongValue>(index).value);\
        else\
            snprintf(msgbuffer,sizeof(msgbuffer),"%s-%s: unknown",PluginDebug::getPluginNameDebug(message->getSenderId()),PluginDebug::getPluginValueNameDebug(message->getSenderId(),message->get(index).getId()));\
       MessageOutput.println(msgbuffer);\
    }\
    }\
  })
#define DBGPRINTMESSAGETAGSLN(level,message) ({\
       char tagbuffer[64];\
       auto keys = message->getMetaData().getKeys();\
       for(auto index : keys) {\
        if(message->getMetaData().isValueType<BoolValue>(index))\
            snprintf(tagbuffer,sizeof(tagbuffer),"TAG %s: bool %d",PluginDebug::getTagNameDebug(static_cast<int>(index)),message->getMetaData().getValueAs<BoolValue>(index).value);\
        else if(message->getMetaData().isValueType<FloatValue>(index))\
            snprintf(tagbuffer,sizeof(tagbuffer),"TAG %s: float %f",PluginDebug::getTagNameDebug(static_cast<int>(index)),message->getMetaData().getValueAs<FloatValue>(index).value);\
        else if(message->getMetaData().isValueType<IntValue>(index))\
            snprintf(tagbuffer,sizeof(tagbuffer),"TAG %s: int %d",PluginDebug::getTagNameDebug(static_cast<int>(index)),message->getMetaData().getValueAs<IntValue>(index).value);\
        else if(message->getMetaData().isValueType<StringValue>(index))\
            snprintf(tagbuffer,sizeof(tagbuffer),"TAG %s: string %s",PluginDebug::getTagNameDebug(static_cast<int>(index)),message->getMetaData().getValueAs<StringValue>(index).value.c_str());\
        else\
            snprintf(tagbuffer,sizeof(tagbuffer),"TAG %s: unknown",PluginDebug::getTagNameDebug(static_cast<int>(index)));\
       MessageOutput.println(tagbuffer);\
    }\
  })
#endif

#endif /* __PLUGINIDS_H__ */
