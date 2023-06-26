#include "Waveshare_Lib/epdif.h"

EpdIf::EpdIf() {};

EpdIf::~EpdIf() {};

uint8_t EpdIf::IfInit()
{
    pinMode(EPD_BUSY_PIN, INPUT);
    pinMode(EPD_RST_PIN, OUTPUT);
    pinMode(EPD_DC_PIN, OUTPUT);

    pinMode(EPD_SCK_PIN, OUTPUT);
    pinMode(EPD_MOSI_PIN, OUTPUT);
    pinMode(EPD_CS_PIN, OUTPUT);

    digitalWrite(EPD_CS_PIN, HIGH);
    digitalWrite(EPD_SCK_PIN, LOW);

    // Serial.begin(115200);

    return 0;
}

void EpdIf::DigitalWrite(uint8_t pin, int value)
{
    digitalWrite(pin, value == 0 ? LOW : HIGH);
}

int EpdIf::DigitalRead(uint8_t pin)
{
    return digitalRead(pin);
}

void EpdIf::DelayMs(uint16_t delaytime)
{
    delay(delaytime);
}

void EpdIf::SpiTransfer(uint8_t data)
{
    digitalWrite(EPD_CS_PIN, GPIO_PIN_RESET);

    for (int i = 0; i < 8; i++) {
        if ((data & 0x80) == 0)
            digitalWrite(EPD_MOSI_PIN, GPIO_PIN_RESET);
        else
            digitalWrite(EPD_MOSI_PIN, GPIO_PIN_SET);

        data <<= 1;
        digitalWrite(EPD_SCK_PIN, GPIO_PIN_SET);
        digitalWrite(EPD_SCK_PIN, GPIO_PIN_RESET);
    }

    digitalWrite(EPD_CS_PIN, GPIO_PIN_SET);
}
