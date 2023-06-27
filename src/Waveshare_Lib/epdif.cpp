#include "Waveshare_Lib/epdif.h"

EpdIf::EpdIf() {};

EpdIf::~EpdIf() {};

void EpdIf::DigitalWrite(uint8_t pin, int value)
{
    digitalWrite(pin, value);
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
    digitalWrite(EPD_CS_PIN, LOW);

    for (int i = 0; i < 8; i++) {
        if ((data & 0x80) == 0)
            digitalWrite(EPD_MOSI_PIN, LOW);
        else
            digitalWrite(EPD_MOSI_PIN, HIGH);

        data <<= 1;
        digitalWrite(EPD_SCK_PIN, HIGH);
        digitalWrite(EPD_SCK_PIN, LOW);
    }

    digitalWrite(EPD_CS_PIN, HIGH);
}

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

    // serial printf
    Serial.begin(115200);

    return 0;
}
