#include "HM_1CH.h"

HM_1CH::HM_1CH(uint64_t serial)
    : HM_Abstract(serial) {};

bool HM_1CH::isValidSerial(uint64_t serial)
{
    return serial >= 0x112100000000 && serial <= 0x112199999999;
}

String HM_1CH::typeName()
{
    return String(F("HM-300, HM-350, HM-400"));
}

const byteAssign_t* HM_1CH::getByteAssignment()
{
    return byteAssignment;
}

const uint8_t HM_1CH::getAssignmentCount()
{
    return sizeof(byteAssignment) / sizeof(byteAssign_t);
}