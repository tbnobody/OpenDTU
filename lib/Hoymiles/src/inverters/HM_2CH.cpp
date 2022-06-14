#include "HM_2CH.h"

bool HM_2CH::isValidSerial(uint64_t serial)
{
    return serial >= 0x114100000000 && serial <= 0x114199999999;
}

String HM_2CH::typeName()
{
    return String(F("HM-600, HM-700, HM-800"));
}

const byteAssign_t* HM_2CH::getByteAssignment()
{
    return byteAssignment;
}