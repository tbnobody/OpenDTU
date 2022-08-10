#include "DevInfoSampleCommand.h"
#include "inverters/InverterAbstract.h"

DevInfoSampleCommand::DevInfoSampleCommand(uint64_t target_address, uint64_t router_address, time_t time)
    : MultiDataCommand(target_address, router_address)
{
    setTime(time);
    setDataType(0x00);
    setTimeout(200);
}

bool DevInfoSampleCommand::handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id)
{
    // Check CRC of whole payload
    if (!MultiDataCommand::handleResponse(inverter, fragment, max_fragment_id)) {
        return false;
    }

    // Move all fragments into target buffer
    uint8_t offs = 0;
    inverter->DevInfo()->clearBufferSample();
    for (uint8_t i = 0; i < max_fragment_id; i++) {
        inverter->DevInfo()->appendFragmentSample(offs, fragment[i].fragment, fragment[i].len);
        offs += (fragment[i].len);
    }
    inverter->DevInfo()->setLastUpdateSample(millis());
    return true;
}