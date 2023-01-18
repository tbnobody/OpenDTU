#ifndef Display_Graphic_H
#define Display_Graphic_H

#include <Arduino.h>
#include <Hoymiles.h>
#include <NetworkSettings.h>
#include <U8g2lib.h>
#include <map>
#include <time.h>

class DisplayGraphicClass {
public:
    DisplayGraphicClass();
    ~DisplayGraphicClass();

    void init(uint8_t type, uint8_t disp_SCL, uint8_t disp_SDA, uint8_t disp_reset);
    void loop();

    bool dispPowerSafe = true;
    bool dispLogo = true;
    uint8_t dispContrast = 60;

private:
    void printText(const char* text, uint8_t line);

    U8G2* _display;

    uint8_t _mExtra;
    uint8_t _display_type = 0;
    uint8_t _display_SCL = SCL;
    uint8_t _display_SDA = SDA;
    uint8_t _display_reset = U8X8_PIN_NONE;
    uint16_t _dispX = 0;
    uint16_t _dispY = 0;
    uint16_t _period = 1000;
    uint16_t _interval = 60000; // interval at which to power save (milliseconds)
    uint32_t _lastPublish = 0;
    uint32_t _previousMillis = 0;
    char _fmtText[32];
};

extern DisplayGraphicClass Display;

#endif
