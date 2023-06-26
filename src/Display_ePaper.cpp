
#include "Display_ePaper.h"
/* Entry point ----------------------------------------------------------------*/

DisplayEPaperClass::DisplayEPaperClass()
{
}

DisplayEPaperClass::~DisplayEPaperClass()
{
    delete epd;
    delete paint;
}

void DisplayEPaperClass::init(DisplayType_t type, uint8_t _CS, uint8_t _DC, uint8_t _RST, uint8_t _BUSY, uint8_t _SCK, uint8_t _MOSI)
{
    // Serial.begin(9600); // start serial handling for text input

    epd = new Epd(_CS, _DC, _RST, _BUSY, _SCK, _MOSI);
    paint = new Paint(image, 200, 200);

    epd->LDirInit(); // initialize epaper
    epd->Clear(); // clear old text/imagery
    epd->DisplayPartBaseWhiteImage(); // lay a base white layer down first
    paint->SetRotate(0);
}

//***************************************************************************
void DisplayEPaperClass::headlineIP()
{
    paint->SetWidth(200); // set display width
    paint->SetHeight(headfootline); // set initial vertical space

    if ((NetworkSettings.isConnected() == true) && (NetworkSettings.localIP() > 0)) {
        snprintf(_fmtText, sizeof(_fmtText), "%s", NetworkSettings.localIP().toString().c_str());
    } else {
        snprintf(_fmtText, sizeof(_fmtText), "no WiFi!!!");
    }

    paint->Clear(COLORED); // darkr background
    x = ((paint->GetWidth() - paint->getStringWidth(_fmtText, Font16.Width)) / 2);
    paint->DrawStringAt(x, 2, _fmtText, &Font16, UNCOLORED); // light texts
    epd->SetFrameMemoryPartial(paint->GetImage(), 0, initial_space + (0 * row_height), paint->GetWidth(), paint->GetHeight());
}
//***************************************************************************
void DisplayEPaperClass::lastUpdatePaged()
{
    paint->SetWidth(200); // set display width
    paint->SetHeight(headfootline); // set initial vertical space

    time_t now = time(nullptr);
    strftime(_fmtText, sizeof(_fmtText), "%d.%m.%Y %H:%M", localtime(&now));

    paint->Clear(COLORED); // darkr background
    x = ((paint->GetWidth() - paint->getStringWidth(_fmtText, Font16.Width)) / 2);
    paint->DrawStringAt(x, 2, _fmtText, &Font16, UNCOLORED); // light text
    epd->SetFrameMemoryPartial(paint->GetImage(), 0, 200 - initial_space - headfootline, paint->GetWidth(), paint->GetHeight());
}
//***************************************************************************
void DisplayEPaperClass::actualPowerPaged(float _totalPower, float _totalYieldDay, float _totalYieldTotal, uint8_t _isprod)
{
    paint->SetWidth(200); // set display width
    paint->SetHeight(actualPower_height); // set initial vertical space

    // actual Production
    if (_totalPower > 9999) {
        snprintf(_fmtText, sizeof(_fmtText), "%.1f kW", (_totalPower / 1000));
    } else if ((_totalPower > 0) && (_totalPower <= 9999)) {
        snprintf(_fmtText, sizeof(_fmtText), "%.0f W", _totalPower);
    } else {
        snprintf(_fmtText, sizeof(_fmtText), "offline");
    }
    paint->Clear(UNCOLORED); // darkr background
    x = ((paint->GetWidth() - paint->getStringWidth(_fmtText, Font24.Width)) / 2);
    paint->DrawStringAt(x, 2, _fmtText, &Font24, COLORED); // light texts
    epd->SetFrameMemoryPartial(paint->GetImage(), 0, initial_space + (1 * row_height), paint->GetWidth(), paint->GetHeight());

    if ((_totalYieldDay > 0) && (_totalYieldTotal > 0)) {
        // Today Production
        if (_totalYieldDay > 9999) {
            snprintf(_fmtText, paint->GetWidth(), "%.1f", (_totalYieldDay / 1000));
            x = ((paint->GetWidth() - paint->getStringWidth(_fmtText, Font24.Width)) / 2);
            paint->DrawStringAt(x, 2, _fmtText, &Font24, COLORED); // light texts

            x = (paint->GetWidth() - 50);
            paint->DrawStringAt(x, 2, "kWh", &Font24, COLORED); // light texts
        } else if (_totalYieldDay <= 9999) {
            snprintf(_fmtText, paint->GetWidth(), "%.1f", (_totalYieldDay));
            x = ((paint->GetWidth() - paint->getStringWidth(_fmtText, Font24.Width)) / 2);
            paint->DrawStringAt(x, 2, _fmtText, &Font24, COLORED); // light texts

            x = (paint->GetWidth() - 38);
            paint->DrawStringAt(x, 2, "Wh", &Font24, COLORED); // light texts
        }
        paint->drawInvertedBitmap(5, 0, myToday, 30, 30, COLORED);
        epd->SetFrameMemoryPartial(paint->GetImage(), 0, initial_space + (2 * row_height), paint->GetWidth(), paint->GetHeight());

        // Total Production
        if (_totalYieldTotal > 9999) {
            snprintf(_fmtText, paint->GetWidth(), "%.1f", (_totalYieldTotal / 1000));
            x = ((paint->GetWidth() - paint->getStringWidth(_fmtText, Font24.Width)) / 2);
            paint->DrawStringAt(x, 2, _fmtText, &Font24, COLORED); // light texts

            x = (paint->GetWidth() - 59);
            paint->DrawStringAt(x, 2, "MWh", &Font24, COLORED); // light texts
        } else if (_totalYieldTotal <= 9999) {
            snprintf(_fmtText, paint->GetWidth(), "%.1f", (_totalYieldTotal));
            x = ((paint->GetWidth() - paint->getStringWidth(_fmtText, Font24.Width)) / 2);
            paint->DrawStringAt(x, 2, _fmtText, &Font24, COLORED); // light texts

            x = (paint->GetWidth() - 50);
            paint->DrawStringAt(x, 2, "kWh", &Font24, COLORED); // light texts
        }
        paint->drawInvertedBitmap(5, 0, mySigma, 30, 30, COLORED);
        epd->SetFrameMemoryPartial(paint->GetImage(), 0, initial_space + (3 * row_height), paint->GetWidth(), paint->GetHeight());

        // Inverter online
        paint->SetWidth(200); // set display width
        paint->SetHeight(20); // set initial vertical space

        snprintf(_fmtText, sizeof(_fmtText), " %d online", _isprod);
        paint->drawInvertedBitmap(5, 0, myWR, 20, 20, COLORED);

        paint->Clear(COLORED); // darkr background
        x = ((paint->GetWidth() - paint->getStringWidth(_fmtText, Font16.Width)) / 2);
        paint->DrawStringAt(x, 2, _fmtText, &Font16, UNCOLORED); // light text
        epd->SetFrameMemoryPartial(paint->GetImage(), 0, initial_space + (4 * row_height), paint->GetWidth(), paint->GetHeight());
    }
}

/* The main loop -------------------------------------------------------------*/
void DisplayEPaperClass::loop(float totalPower, float totalYieldDay, float totalYieldTotal, uint8_t isprod)
{
    headlineIP(); // print header text
    lastUpdatePaged();
    actualPowerPaged(totalPower, totalYieldDay, totalYieldTotal, isprod);

    epd->DisplayPartFrame(); // display new text
}

DisplayEPaperClass DisplayEPaper;