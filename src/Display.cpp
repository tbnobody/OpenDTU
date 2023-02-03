#include "Display.h"
#include "Display_Mono.h"
#include "Display_ePaper.h"

#include <Hoymiles.h>

/*************** HSPI Belegung *******************************
MISO(Busy) 12   // ePaper Busy indicator (SPI MISO aquivalent)
RST 26          // ePaper Reset switch
DC 27           // ePaper Data/Command selection
CS(SS) 15       // SPI Channel Chip Selection for ePaper
SCK(CLK) 14     // SPI Channel Click
MOSI(DIN) 13    // SPI Channel MOSI Pin
*************************************************************/

void DisplayClass::init(DisplayType_t _type)
{
    _type = DisplayType_t::ePaper154;

    _display_type = _type;
    if (_type == DisplayType_t::None)
    {
        return;
    }
    else if ((_type == SSD1306) || (_type == SH1106))
    {
        // esp8266: SCL = 5, SDA = 4
        // esp32  : SCL = 22, SDA = 21
        uint8_t reset = U8X8_PIN_NONE;
        DisplayMono.init(_type, SCL, SDA, reset);
        DisplayMono.enablePowerSafe = enablePowerSafe;
        DisplayMono.enableScreensaver = enableScreensaver;
        DisplayMono.contrast = contrast;
        _period = 1000;
    }
#if defined(ESP32)
    else if (_type == ePaper154)
    {
        /************ Definition start ***********/
        uint8_t BUSY_PIN = 12;
        uint8_t RST_PIN = 26;
        uint8_t DC_PIN = 27;
        uint8_t CS_PIN = 15;
        uint8_t CLK_PIN = 14;
        uint8_t MOSI_PIN = 13;
        /************ Definition ende ***********/

        DisplayEPaper.init(_type, CS_PIN, DC_PIN, RST_PIN, BUSY_PIN, CLK_PIN, MOSI_PIN);
        _period = 60000; // Achtung, max 65535
        counterEPaper = 0;
    }
#endif
}

void DisplayClass::loop()
{
    if (_display_type == DisplayType_t::None)
    {
        return;
    }

    if ((millis() - _lastDisplayUpdate) > _period)
    {
        float totalPower = 0;
        float totalYieldDay = 0;
        float totalYieldTotal = 0;

        uint8_t isprod = 0;

        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++)
        {
            auto inv = Hoymiles.getInverterByPos(i);
            if (inv == nullptr)
            {
                continue;
            }

            if (inv->isProducing() && inv->isReachable())
            {
                isprod++;
            }

            totalPower += (inv->Statistics()->getChannelFieldValue(CH0, FLD_PAC) * inv->isReachable());
            totalYieldDay += (inv->Statistics()->getChannelFieldValue(CH0, FLD_YD));
            totalYieldTotal += (inv->Statistics()->getChannelFieldValue(CH0, FLD_YT));
        }

        if ((_display_type == SSD1306) || (_display_type == SH1106))
        {
            DisplayMono.loop(totalPower, totalYieldDay, totalYieldTotal, isprod);
        }
#if defined(ESP32)
        else if (_display_type == ePaper154)
        {

            DisplayEPaper.loop(totalPower, totalYieldDay, totalYieldTotal, isprod);
            counterEPaper++;
        }
#endif
        _lastDisplayUpdate = millis();
    }

    if (counterEPaper > 480)
    {
        DisplayEPaper.fullRefresh();
        counterEPaper = 0;
    }
}

DisplayClass Display;