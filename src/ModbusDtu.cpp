#include "ModbusDTU.h"

ModbusIP mb;

ModbusDtuClass ModbusDtu;

void ModbusDtuClass::init()
{
    mb.server();	
    const CONFIG_T& config = Configuration.get();
    mb.addHreg(0x2000, (config.Dtu_Serial >> 32) & 0xFFFF);
    mb.addHreg(0x2001, (config.Dtu_Serial >> 16) & 0xFFFF);
    mb.addHreg(0x2002, (config.Dtu_Serial) & 0xFFFF);
    mb.addHreg(0x2003, 0);
    mb.addHreg(0x2004, 0);
    mb.addHreg(0x2005, 0);

    _channels = 0;
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);
        if (inv == nullptr) {
            continue;
        }
        _channels +=inv->Statistics()->getChannelCount();
    }
    mb.addHreg(0x200, _channels);
    
    for (uint8_t i = 0; i <= _channels; i++) {
        for(uint8_t j = 0; j < 20; j++) {
            mb.addHreg(i* 20 + 0x1000 + j, 0);
        }
        yield();
    }
    
}

void ModbusDtuClass::loop()
{
    
    uint8_t chan = 0;
    if (millis() - _lastPublish > 2000 && Hoymiles.getRadio()->isIdle()) {
        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);
            if (inv == nullptr) {
            continue;
            }
            // Loop all channels
            for (uint8_t c = 0; c <= inv->Statistics()->getChannelCount(); c++) {
                if (c > 0) {
                    uint64_t serialInv = inv->serial();
                    mb.Hreg(chan* 20 + 0x1000, 0x0C00 + ((serialInv >> 40) & 0xFF));
                    mb.Hreg(chan* 20 + 0x1001, (serialInv >> 24) & 0xFFFF);
                    mb.Hreg(chan* 20 + 0x1002, (serialInv >> 8) & 0xFFFF);
                    mb.Hreg(chan* 20 + 0x1003, (serialInv << 8) + c);
                    if(inv->Statistics()->getChannelMaxPower(c-1)>0 && inv->Statistics()->getChannelFieldValue(c, FLD_IRR) < 400){
                        mb.Hreg(chan* 20 + 0x1004, (uint16_t)(inv->Statistics()->getChannelFieldValue(c, FLD_UDC)*10));
                        mb.Hreg(chan* 20 + 0x1005, (uint16_t)(inv->Statistics()->getChannelFieldValue(c, FLD_IDC)*100));
                        mb.Hreg(chan* 20 + 0x1008, (uint16_t)(inv->Statistics()->getChannelFieldValue(c, FLD_PDC)*10));
                    } else if (inv->Statistics()->getChannelMaxPower(c-1)==0)
                    {
                        mb.Hreg(chan* 20 + 0x1004, (uint16_t)(inv->Statistics()->getChannelFieldValue(c, FLD_UDC)*10));
                        mb.Hreg(chan* 20 + 0x1005, (uint16_t)(inv->Statistics()->getChannelFieldValue(c, FLD_IDC)*100));
                        mb.Hreg(chan* 20 + 0x1008, (uint16_t)(inv->Statistics()->getChannelFieldValue(c, FLD_PDC)*10));
                    }
                    
                    mb.Hreg(chan* 20 + 0x1006, (uint16_t)(inv->Statistics()->getChannelFieldValue(0, FLD_UAC)*10));
                    mb.Hreg(chan* 20 + 0x1007, (uint16_t)(inv->Statistics()->getChannelFieldValue(0, FLD_F)*100));
                    mb.Hreg(chan* 20 + 0x1009, (uint16_t)(inv->Statistics()->getChannelFieldValue(c, FLD_YD)));
                    mb.Hreg(chan* 20 + 0x100A, ((uint16_t)(((uint32_t)(inv->Statistics()->getChannelFieldValue(c, FLD_YT)*1000)) >> 16)) & 0xFFFF);
                    mb.Hreg(chan* 20 + 0x100B, (((uint16_t)(inv->Statistics()->getChannelFieldValue(c, FLD_YT)*1000))) & 0xFFFF);
                    mb.Hreg(chan* 20 + 0x100C, (uint16_t)(inv->Statistics()->getChannelFieldValue(0, FLD_T)*10));
                    mb.Hreg(chan* 20 + 0x100D, 3);
                    mb.Hreg(chan* 20 + 0x100E, 0);
                    mb.Hreg(chan* 20 + 0x100F, 0);
                    mb.Hreg(chan* 20 + 0x1010, 0x0107);
                    mb.Hreg(chan* 20 + 0x1011, 0);
                    mb.Hreg(chan* 20 + 0x1012, 0);
                    mb.Hreg(chan* 20 + 0x1013, 0);
                    chan++;
                }
            }
        }
    _lastPublish = millis();
    }
    yield();
    mb.task();
}