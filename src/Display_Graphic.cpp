#include "Display_Graphic.h"

static uint8_t bmp_logo[] PROGMEM = {
    B00000000, B00000000, // ................
    B11101100, B00110111, // ..##.######.##..
    B11101100, B00110111, // ..##.######.##..
    B11100000, B00000111, // .....######.....
    B11010000, B00001011, // ....#.####.#....
    B10011000, B00011001, // ...##..##..##...
    B10000000, B00000001, // .......##.......
    B00000000, B00000000, // ................
    B01111000, B00011110, // ...####..####...
    B11111100, B00111111, // ..############..
    B01111100, B00111110, // ..#####..#####..
    B00000000, B00000000, // ................
    B11111100, B00111111, // ..############..
    B11111110, B01111111, // .##############.
    B01111110, B01111110, // .######..######.
    B00000000, B00000000 // ................
};

static uint8_t bmp_arrow[] PROGMEM = {
    B00000000, B00011100, B00011100, B00001110, B00001110, B11111110, B01111111,
    B01110000, B01110000, B00110000, B00111000, B00011000, B01111111, B00111111,
    B00011110, B00001110, B00000110, B00000000, B00000000, B00000000, B00000000
};

std::map<int, std::function<U8G2*()>> display_types = {
    { 0, []() { return new U8G2_PCD8544_84X48_F_4W_HW_SPI(U8G2_R0, /*cs*/ 5, /*dc*/ 4, /*reset*/ 16); } },
    { 1, []() { return new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, /*reset*/ U8X8_PIN_NONE, /*clock*/ SCL, /*data*/ SDA); } },
    { 2, []() { return new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, /*reset*/ U8X8_PIN_NONE, /*clock*/ SCL, /*data*/ SDA); } },
};

DisplayGraphicClass::DisplayGraphicClass()
{
    init(_display_type);
}

DisplayGraphicClass::~DisplayGraphicClass()
{
    delete _display;
}

void DisplayGraphicClass::init(uint8_t type)
{
    _display_type = type;
    int const_type = type - 1;
    if (const_type >= 0) {
        auto constructor = display_types[const_type];
        _display = constructor();
        _display->begin();
    }
}

void DisplayGraphicClass::printText(const char* text, uint8_t line)
{
    // get the width and height of the display
    uint16_t maxWidth = _display->getWidth();
    uint16_t maxHeight = _display->getHeight();

    // pxMovement +x (0 - 6 px)
    uint8_t ex = (_mExtra % 7);

    // set the font size based on the display size
    switch (line) {
    case 1:
        if (maxWidth > 120 && maxHeight > 60) {
            _display->setFont(u8g2_font_ncenB14_tr); // large display
        } else {
            _display->setFont(u8g2_font_logisoso16_tr); // small display
        }
        break;
    case 4:
        if (maxWidth > 120 && maxHeight > 60) {
            _display->setFont(u8g2_font_5x8_tr); // large display
        } else {
            _display->setFont(u8g2_font_5x8_tr); // small display
        }
        break;
    default:
        if (maxWidth > 120 && maxHeight > 60) {
            _display->setFont(u8g2_font_ncenB10_tr); // large display
        } else {
            _display->setFont(u8g2_font_5x8_tr); // small display
        }
        break;
    }

    // get the font height, to calculate the textheight
    _dispY += (_display->getMaxCharHeight()) + 1;

    // calculate the starting position of the text
    if (line == 1) {
        _dispX = 20 + ex;
    } else {
        _dispX = 5 + ex;
    }

    // draw the Text, on the calculated pos
    _display->drawStr(_dispX, _dispY, text);
}

void DisplayGraphicClass::loop()
{
    if (_display_type == 0) {
        return;
    }

    if ((millis() - _lastPublish) > _period) {
        float totalPower = 0;
        float totalYieldDay = 0;
        float totalYieldTotal = 0;

        uint8_t isprod = 0;

        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);
            if (inv == nullptr) {
                continue;
            }

            if (inv->isProducing()) {
                isprod++;
            }

            totalPower += inv->Statistics()->getChannelFieldValue(CH0, FLD_PAC);
            totalYieldDay += inv->Statistics()->getChannelFieldValue(CH0, FLD_YD);
            totalYieldTotal += inv->Statistics()->getChannelFieldValue(CH0, FLD_YT);
        }

        _display->clearBuffer();

        // set Contrast of the Display to raise the lifetime
        _display->setContrast(dispContrast);

        //=====> Logo and Lighting ==========
        //   pxMovement +x (0 - 6 px)
        uint8_t ex = (_mExtra % 7);
        if (isprod > 0) {
            _display->drawXBMP(5 + ex, 1, 8, 17, bmp_arrow);
            if (dispLogo)
                _display->drawXBMP(_display->getWidth() - 24 + ex, 2, 16, 16, bmp_logo);
        }
        //<=======================

        //=====> Actual Production ==========
        if ((totalPower > 0) && (isprod > 0)) {
            _display->setPowerSave(false);
            if (totalPower > 999) {
                snprintf(_fmtText, sizeof(_fmtText), "%2.1f kW", (totalPower / 1000));
            } else {
                snprintf(_fmtText, sizeof(_fmtText), "%3.0f W", totalPower);
            }
            printText(_fmtText, 1);
            _previousMillis = millis();
        }
        //<=======================

        //=====> Offline ===========
        else {
            printText("offline", 1);
            // check if it's time to enter power saving mode
            if (millis() - _previousMillis >= (_interval * 2) && (dispPowerSafe)) {
                _display->setPowerSave(true);
            }
        }
        //<=======================

        //=====> Today & Total Production =======
        snprintf(_fmtText, sizeof(_fmtText), "today: %4.0f Wh", totalYieldDay);
        printText(_fmtText, 2);

        snprintf(_fmtText, sizeof(_fmtText), "total: %.1f kWh", totalYieldTotal);
        printText(_fmtText, 3);
        //<=======================

        //=====> IP or Date-Time ========
        if (!(_mExtra % 10) && NetworkSettings.localIP()) {
            printText(NetworkSettings.localIP().toString().c_str(), 4);
        } else {
            // Get current time
            time_t now = time(nullptr);
            strftime(_fmtText, sizeof(_fmtText), "%a %d.%m.%Y %H:%M", localtime(&now));
            printText(_fmtText, 4);
        }
        _display->sendBuffer();

        _dispX = 0;
        _dispY = 0;
        _mExtra++;
        _lastPublish = millis();
    }
}

DisplayGraphicClass Display;