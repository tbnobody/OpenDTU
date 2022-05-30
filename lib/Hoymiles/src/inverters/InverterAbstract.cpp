#include "InverterAbstract.h"
#include <cstring>

void InverterAbstract::setSerial(uint64_t serial)
{
    _serial.u64 = serial;
}

uint64_t InverterAbstract::serial()
{
    return _serial.u64;
}

void InverterAbstract::setName(const char* name)
{
    uint8_t len = strlen(name);
    strncpy(_name, name, (len > MAX_NAME_LENGTH) ? MAX_NAME_LENGTH : len);
}

const char* InverterAbstract::name()
{
    return _name;
}
