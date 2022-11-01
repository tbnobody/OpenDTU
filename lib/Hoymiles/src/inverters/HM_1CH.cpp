#include "HM_1CH.h"

HM_1CH::HM_1CH(uint64_t serial)
    : HM_Abstract(serial) {};

bool HM_1CH::isValidSerial(uint64_t serial)
{
    // serial >= 0x112100000000 && serial <= 0x112199999999

    uint8_t preId[2];
    preId[0] = (uint8_t)(serial >> 40);
    preId[1] = (uint8_t)(serial >> 32);

    if ((uint8_t)(((((uint16_t)preId[0] << 8) | preId[1]) >> 4) & 0xff) == 0x12) {
        return true;
    }

    if ((((preId[1] & 0xf0) == 0x10) || ((preId[1] & 0xf0) == 0x20))
        && (((preId[0] == 0x10) && (preId[1] == 0x22)) || ((preId[0] == 0x11) && (preId[1] == 0x21)))) {
        return true;
    }

    return false;
}

String HM_1CH::typeName()
{
    return F("HM-300, HM-350, HM-400");
}

const byteAssign_t* HM_1CH::getByteAssignment()
{
    return byteAssignment;
}

uint8_t HM_1CH::getAssignmentCount()
{
    return sizeof(byteAssignment) / sizeof(byteAssign_t);
}