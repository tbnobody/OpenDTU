#include "Display.h"
#include "Display_Mono.h"
#include "Display_ePaper.h"

#include <Hoymiles.h>

void DisplayClass::init(DisplayType_t _type, uint8_t _data, uint8_t _clk, uint8_t _cs, uint8_t _reset, uint8_t _busy, uint8_t _dc)
{
    //************** HSPI Belegung ********************************
    // MISO(Busy)  12   // ePaper Busy indicator (SPI MISO aquivalent)
    // RST         26   // ePaper Reset switch
    // DC          27   // ePaper Data/Command selection
    // CS(SS)      15   // SPI Channel Chip Selection for ePaper
    // SCK(CLK)    14   // SPI Channel Click
    // MOSI(DIN)   13   // SPI Channel MOSI Pin
    //*************************************************************

    //*************** I2C Belegung ********************************
    // esp8266: SCL = 5, SDA = 4
    // esp32  : SCL = 22, SDA = 21
    // _SCL = _clk; _SDA = _data;
    //*************************************************************

    _display_type = _type;
    if (_type == DisplayType_t::None) {
        return;
    } else if ((_type == PCD8544) || (_type == SSD1306) || (_type == SH1106)) {

        switch (rotation) {
        case 0:
            DisplayMono.disp_rotation = U8G2_R0;
            break;
        case 1:
            DisplayMono.disp_rotation = U8G2_R1;
            break;
        case 2:
            DisplayMono.disp_rotation = U8G2_R2;
            break;
        case 3:
            DisplayMono.disp_rotation = U8G2_R3;
            break;
        }

        DisplayMono.enablePowerSafe = enablePowerSafe;
        DisplayMono.enableScreensaver = enableScreensaver;
        DisplayMono.contrast = contrast;

        DisplayMono.init(_type, _cs, _dc, _reset, _busy, _clk, _data);
    } else if (_type == ePaper154) {

        DisplayEPaper.displayRotation = rotation;
        counterEPaper = 0;

        DisplayEPaper.init(_type, _cs, _dc, _reset, _busy, _clk, _data);
    }
}

void DisplayClass::loop()
{
    if (_display_type == DisplayType_t::None) {
        return;
    }

    if ((millis() - _lastDisplayUpdate) > period) {
        float totalPower = 0;
        float totalYieldDay = 0;
        float totalYieldTotal = 0;

        uint8_t isprod = 0;

        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);
            if (inv == nullptr) {
                continue;
            }

            if (inv->isProducing() && inv->isReachable()) {
                isprod++;
            }

            for (auto& c : inv->Statistics()->getChannelsByType(TYPE_AC)) {
                totalPower += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_PAC) * inv->isReachable();
                totalYieldDay += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_YD);
                totalYieldTotal += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_YT);
            }
        }

        if ((_display_type == PCD8544) || (_display_type == SSD1306) || (_display_type == SH1106)) {
            DisplayMono.loop(totalPower, totalYieldDay, totalYieldTotal, isprod);
        } else if (_display_type == ePaper154) {

            DisplayEPaper.loop(totalPower, totalYieldDay, totalYieldTotal, isprod);
            counterEPaper++;
        }
        _lastDisplayUpdate = millis();
    }

    if (counterEPaper > 480) {
        DisplayEPaper.fullRefresh();
        counterEPaper = 0;
    }
}

DisplayClass Display;