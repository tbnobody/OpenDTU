#include "HM_Abstract.h"
#include "HoymilesRadio.h"
#include "crc.h"

HM_Abstract::HM_Abstract(uint64_t serial)
    : InverterAbstract(serial) {};

bool HM_Abstract::getStatsRequest(inverter_transaction_t* payload)
{
    time_t now;
    time(&now);

    memset(payload->payload, 0, MAX_RF_PAYLOAD_SIZE);

    payload->target.u64 = serial();
    payload->mainCmd = 0x15;
    payload->subCmd = 0x80;
    payload->timeout = 200;
    payload->len = 16;

    payload->payload[0] = 0x0b;
    payload->payload[1] = 0x00;

    HoymilesRadio::u32CpyLittleEndian(&payload->payload[2], now); // sets the 4 following elements {2, 3, 4, 5}
    payload->payload[9] = 0x05;

    uint16_t crc = crc16(&payload->payload[0], 14);
    payload->payload[14] = (crc >> 8) & 0xff;
    payload->payload[15] = (crc)&0xff;

    return now > 0;
}