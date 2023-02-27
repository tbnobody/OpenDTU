#include "Display_ePaper.h"
#include "imagedata.h"
#include <NetworkSettings.h>

static const uint32_t spiClk = 4000000; // 4 MHz

#if defined(ESP32) && defined(USE_HSPI_FOR_EPD)
SPIClass hspi(HSPI);
#endif

std::map<DisplayType_t, std::function<GxEPD2_GFX*(uint8_t, uint8_t, uint8_t, uint8_t)>> _ePaperTypes = {
    // DEPG0150BN 200x200, SSD1681, TTGO T5 V2.4.1
    { DisplayType_t::ePaper154, [](uint8_t _CS, uint8_t _DC, uint8_t _RST, uint8_t _BUSY) { return new GxEPD2_BW<GxEPD2_150_BN, GxEPD2_150_BN::HEIGHT>(GxEPD2_150_BN(_CS, _DC, _RST, _BUSY)); } },
    // GDEW027C44   2.7 " b/w/r 176x264, IL91874
    //{DisplayType_t::ePaper270, [](uint8_t _CS, uint8_t _DC, uint8_t _RST, uint8_t _BUSY)
    // F { return new GxEPD2_3C<GxEPD2_270c, GxEPD2_270c::HEIGHT>(GxEPD2_270c(_CS, _DC, _RST, _BUSY)); }},
};

DisplayEPaperClass::DisplayEPaperClass()
{
}

DisplayEPaperClass::~DisplayEPaperClass()
{
    delete _display;
}
//***************************************************************************
void DisplayEPaperClass::init(DisplayType_t type, uint8_t _CS, uint8_t _DC, uint8_t _RST, uint8_t _BUSY, uint8_t _SCK, uint8_t _MOSI)
{
    if (type > DisplayType_t::None) {
        Serial.begin(115200);
        auto constructor = _ePaperTypes[type];
        _display = constructor(_CS, _DC, _RST, _BUSY);
        hspi.begin(_SCK, _BUSY, _MOSI, _CS);

#if defined(ESP32) && defined(USE_HSPI_FOR_EPD)
        _display->epd2.selectSPI(hspi, SPISettings(spiClk, MSBFIRST, SPI_MODE0));
#endif
        _display->init(115200, true, 2, false);
        _display->setRotation(displayRotation);
        _display->setFullWindow();

        // Logo
        _display->fillScreen(GxEPD_BLACK);
        //_display->drawBitmap(0, 0, AhoyLogo, 200, 200, GxEPD_WHITE);
        _display->drawBitmap(0, 0, OpenDTULogo, 200, 200, GxEPD_WHITE);
        while (_display->nextPage())
            ;

        // clean the screen
        delay(2000);
        _display->fillScreen(GxEPD_WHITE);
        while (_display->nextPage())
            ;

        headlineIP();

        // call the PowerPage to change the PV Power Values
        actualPowerPaged(0, 0, 0, 0);
    }
}
//***************************************************************************
void DisplayEPaperClass::fullRefresh()
{
    // screen complete black
    _display->fillScreen(GxEPD_BLACK);
    while (_display->nextPage())
        ;
    delay(2000);
    // screen complete white
    _display->fillScreen(GxEPD_WHITE);
    while (_display->nextPage())
        ;
}
//***************************************************************************
void DisplayEPaperClass::headlineIP()
{
    int16_t tbx, tby;
    uint16_t tbw, tbh;

    _display->setFont(&FreeSans9pt7b);
    _display->setTextColor(GxEPD_WHITE);

    _display->setPartialWindow(0, 0, _display->width(), headfootline);
    _display->fillScreen(GxEPD_BLACK);
    do {
        if ((NetworkSettings.isConnected() == true) && (NetworkSettings.localIP() > 0)) {
            snprintf(_fmtText, sizeof(_fmtText), "%s", NetworkSettings.localIP().toString().c_str());
        } else {
            snprintf(_fmtText, sizeof(_fmtText), "WiFi not connected");
        }
        _display->getTextBounds(_fmtText, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t x = ((_display->width() - tbw) / 2) - tbx;

        _display->setCursor(x, (headfootline - 2));
        _display->println(_fmtText);
    } while (_display->nextPage());
}
//***************************************************************************
void DisplayEPaperClass::lastUpdatePaged()
{
    int16_t tbx, tby;
    uint16_t tbw, tbh;

    _display->setFont(&FreeSans9pt7b);
    _display->setTextColor(GxEPD_WHITE);

    _display->setPartialWindow(0, _display->height() - headfootline, _display->width(), headfootline);
    _display->fillScreen(GxEPD_BLACK);
    do {
        time_t now = time(nullptr);
        strftime(_fmtText, sizeof(_fmtText), "%d.%m.%Y %H:%M", localtime(&now));

        _display->getTextBounds(_fmtText, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t x = ((_display->width() - tbw) / 2) - tbx;

        _display->setCursor(x, (_display->height() - 3));
        _display->println(_fmtText);
    } while (_display->nextPage());
}
//***************************************************************************
void DisplayEPaperClass::actualPowerPaged(float _totalPower, float _totalYieldDay, float _totalYieldTotal, uint8_t _isprod)
{
    int16_t tbx, tby;
    uint16_t tbw, tbh, x, y;

    _display->setFont(&FreeSans24pt7b);
    _display->setTextColor(GxEPD_BLACK);

    _display->setPartialWindow(0, headfootline, _display->width(), _display->height() - (headfootline * 2));
    _display->fillScreen(GxEPD_WHITE);
    do {
        if (_totalPower > 9999) {
            snprintf(_fmtText, sizeof(_fmtText), "%.1f kW", (_totalPower / 10000));
            _changed = true;
        } else if ((_totalPower > 0) && (_totalPower <= 9999)) {
            snprintf(_fmtText, sizeof(_fmtText), "%.0f W", _totalPower);
            _changed = true;
        } else {
            snprintf(_fmtText, sizeof(_fmtText), "offline");
        }
        _display->getTextBounds(_fmtText, 0, 0, &tbx, &tby, &tbw, &tbh);
        x = ((_display->width() - tbw) / 2) - tbx;
        _display->setCursor(x, headfootline + tbh + 10);
        _display->print(_fmtText);

        _display->setFont(&FreeSans12pt7b);
        y = _display->height() / 2;
        _display->setCursor(0, y);
        _display->print("today:");
        snprintf(_fmtText, _display->width(), "%.0f", _totalYieldDay);
        _display->getTextBounds(_fmtText, 0, 0, &tbx, &tby, &tbw, &tbh);
        x = ((_display->width() - tbw) / 2) - tbx;
        _display->setCursor(x, y);
        _display->print(_fmtText);
        _display->setCursor(_display->width() - 33, y);
        _display->println("Wh");

        y = y + tbh + 7;
        _display->setCursor(0, y);
        _display->print("total:");
        snprintf(_fmtText, _display->width(), "%.1f", _totalYieldTotal);
        _display->getTextBounds(_fmtText, 0, 0, &tbx, &tby, &tbw, &tbh);
        x = ((_display->width() - tbw) / 2) - tbx;
        _display->setCursor(x, y);
        _display->print(_fmtText);
        _display->setCursor(_display->width() - 45, y);
        _display->println("kWh");

        _display->setCursor(0, _display->height() - (headfootline + 10));
        snprintf(_fmtText, sizeof(_fmtText), "#%d Inverter online", _isprod);
        _display->println(_fmtText);

    } while (_display->nextPage());
}
//***************************************************************************
void DisplayEPaperClass::loop(float totalPower, float totalYieldDay, float totalYieldTotal, uint8_t isprod)
{
    // check if the IP has changed
    if (_settedIP != NetworkSettings.localIP().toString().c_str()) {
        // save the new IP and call the Headline Funktion to adapt the Headline
        _settedIP = NetworkSettings.localIP().toString().c_str();
        headlineIP();
    }

    // call the PowerPage to change the PV Power Values
    actualPowerPaged(totalPower, totalYieldDay, totalYieldTotal, isprod);

    // if there was an change and the Inverter is producing set a new Timestam in the footline
    if ((isprod > 0) && (_changed)) {
        _changed = false;
        lastUpdatePaged();
    }

    _display->powerOff();
}
//***************************************************************************
DisplayEPaperClass DisplayEPaper;