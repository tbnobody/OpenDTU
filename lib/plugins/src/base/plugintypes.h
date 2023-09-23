#pragma once

#include "generic.hpp"
#include "Arduino.h"

enum TYPEIDS { 
    DUMMY, 
    FLOATVALUE_TYPE,INTVALUE_TYPE,LONGVALUE_TYPE,BOOLVALUE_TYPE,STRINGVALUE_TYPE,
    PLUGINMESSAGE_TYPE, MQTTMESSAGE_TYPE, INVERTERMESSAGE_TYPE, METERMESSAGE_TYPE, DEMOMESSAGE_TYPE, POWERCONTROLMESSAGE_TYPE
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
