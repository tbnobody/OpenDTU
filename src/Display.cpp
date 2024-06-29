#include "Display.h"
#include "Display_Graphic.h"
#include "Display_ePaper.h"

void DisplayClass::init(Scheduler& scheduler, DisplayType_t _type, uint8_t _data, uint8_t _clk, uint8_t _cs, uint8_t _reset, uint8_t _busy, uint8_t _dc)
{
    //************** HSPI Belegung ********************************
    // MISO(Busy)  12   // ePaper Busy indicator (SPI MISO aquivalent)
    // RST         26   // ePaper Reset switch
    // DC          27   // ePaper Data/Command selection
    // CS(SS)      15   // SPI Channel Chip Selection for ePaper
    // SCK(CLK)    14   // SPI Channel Click
    // MOSI(DIN)   13   // SPI Channel MOSI Pin
    //*************************************************************

    //************** SPI Belegung Fusion V2 *************************
    // MISO(Busy)  14   // ePaper Busy indicator (SPI MISO aquivalent)
    // RST         13   // ePaper Reset switch
    // DC          12   // ePaper Data/Command selection
    // CS(SS)      11   // SPI Channel Chip Selection for ePaper
    // SCK(CLK)    10   // SPI Channel Click
    // MOSI(DIN)   9   // SPI Channel MOSI Pin
    //*************************************************************

    //*************** I2C Belegung ********************************
    // esp8266: SCL = 5, SDA = 4
    // esp32  : SCL = 22, SDA = 21
    // _SCL = _clk; _SDA = _data;
    //*************************************************************

    _display_type = _type;
    if (_type == DisplayType_t::None) {
        return;
    } else if ((_type == PCD8544) || (_type == SSD1306) || (_type == SH1106) || (_type == SSD1309)) {
        DisplayGraphic.init(scheduler, _type, _data, _clk, _cs, _reset);

    } else if (_type == ePaper154) {
        _counterEPaper = 0;

        DisplayEPaper.init(scheduler, _type, _cs, _dc, _reset, _busy, _clk, _data);
    }
}

void DisplayClass::setLanguage(uint8_t language)
{
    if (_display_type == DisplayType_t::None) {
        return;
    } else if ((_display_type == PCD8544) || (_display_type == SSD1306) || (_display_type == SH1106) || (_display_type == SSD1309)) {
        DisplayGraphic.setLanguage(language);
    } else if (_display_type == ePaper154) {
        DisplayEPaper.setLanguage(language);
    }
}

void DisplayClass::setContrast(uint8_t contrast)
{
    if (_display_type == DisplayType_t::None) {
        return;
    } else if ((_display_type == PCD8544) || (_display_type == SSD1306) || (_display_type == SH1106) || (_display_type == SSD1309)) {
        DisplayGraphic.setContrast(contrast);
    } else if (_display_type == ePaper154) {
        return;
    }
}

void DisplayClass::setUpdatePeriod(uint16_t updatePeriod)
{
    if (_display_type == DisplayType_t::None) {
        return;
    }

    if (updatePeriod > 999) {
        _settedUpdatePeriod = updatePeriod;
    } else {
        _settedUpdatePeriod = 10000;
    }
}

void DisplayClass::setOrientation(uint8_t rotation)
{
    if (_display_type == DisplayType_t::None) {
        return;
    } else if ((_display_type == PCD8544) || (_display_type == SSD1306) || (_display_type == SH1106) || (_display_type == SSD1309)) {
        DisplayGraphic.setOrientation(rotation);
    } else if (_display_type == ePaper154) {
        DisplayEPaper.setOrientation(rotation);
    }
}

void DisplayClass::setEnablePowerSafe(bool display_PowerSafe)
{
    if (_display_type == DisplayType_t::None) {
        return;
    } else if ((_display_type == PCD8544) || (_display_type == SSD1306) || (_display_type == SH1106) || (_display_type == SSD1309)) {
        DisplayGraphic.enablePowerSafe = display_PowerSafe;
    } else if (_display_type == ePaper154) {
        return;
    }
}
void DisplayClass::setEnableScreensaver(bool display_ScreenSaver)
{
    if (_display_type == DisplayType_t::None) {
        return;
    } else if ((_display_type == PCD8544) || (_display_type == SSD1306) || (_display_type == SH1106) || (_display_type == SSD1309)) {
        DisplayGraphic.enableScreensaver = display_ScreenSaver;
    } else if (_display_type == ePaper154) {
        return;
    }
}

void DisplayClass::setDiagramMode(DiagramMode_t mode)
{
    if (_display_type == DisplayType_t::None) {
        return;
    } else if ((_display_type == PCD8544) || (_display_type == SSD1306) || (_display_type == SH1106) || (_display_type == SSD1309)) {
        DisplayGraphic.setDiagramMode(mode);
    } else if (_display_type == ePaper154) {
        return;
    }
}

void DisplayClass::setStartupDisplay()
{
    if (_display_type == DisplayType_t::None) {
        return;
    } else if ((_display_type == PCD8544) || (_display_type == SSD1306) || (_display_type == SH1106) || (_display_type == SSD1309)) {
        DisplayGraphic.setStartupDisplay();
    } else if (_display_type == ePaper154) {
        return;
    }
}

void DisplayClass::DiagramUpdatePeriod()
{
    if (_display_type == DisplayType_t::None) {
        return;
    } else if ((_display_type == PCD8544) || (_display_type == SSD1306) || (_display_type == SH1106) || (_display_type == SSD1309)) {
        DisplayGraphic.Diagram().updatePeriod();
    } else if (_display_type == ePaper154) {
        return;
    }
}

void DisplayClass::setStatus(bool turnOn)
{
    _displayTurnedOn = turnOn;
}

DisplayClass Display;