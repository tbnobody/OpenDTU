#include "HM_4CH.h"

bool HM_4CH::isValidSerial(uint64_t serial)
{
    return serial >= 0x116100000000 && serial <= 0x116199999999;
}

String HM_4CH::typeName()
{
    return String(F("HM-1200, HM-1500"));
}

const byteAssign_t* HM_4CH::getByteAssignment()
{
    return byteAssignment;
}

const uint8_t HM_4CH::getAssignmentCount()
{
    return sizeof(byteAssignment) / sizeof(byteAssign_t);
}