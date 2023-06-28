// Display Library for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// based on Demo Example from Good Display: https://www.good-display.com/product/442.html
// Panel: GDEY073D46 : https://www.good-display.com/product/442.html
// Controller: unknown
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2

#include "GxEPD2_730c_GDEY073D46.h"

GxEPD2_730c_GDEY073D46::GxEPD2_730c_GDEY073D46(int16_t cs, int16_t dc, int16_t rst, int16_t busy) :
  GxEPD2_EPD(cs, dc, rst, busy, LOW, 60000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate)
{
  _paged = false;
}

void GxEPD2_730c_GDEY073D46::clearScreen(uint8_t value)
{
  clearScreen(value, 0xFF);
}

void GxEPD2_730c_GDEY073D46::clearScreen(uint8_t black_value, uint8_t color_value)
{
  writeScreenBuffer(black_value, color_value);
  _Update_Full();
}

void GxEPD2_730c_GDEY073D46::writeScreenBuffer(uint8_t value)
{
  writeScreenBuffer(value, 0xFF);
}

void GxEPD2_730c_GDEY073D46::writeScreenBuffer(uint8_t black_value, uint8_t color_value)
{
  _initial_write = false; // initial full screen buffer clean done
  _Init_Full();
  _writeCommand(0x10);
  _startTransfer();
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 2; i++)
  {
    //_transfer(0x11);
    _transfer(0xFF == black_value ? 0x11 : black_value);
  }
  _endTransfer();
}

void GxEPD2_730c_GDEY073D46::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  //Serial.print("writeImage("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if (_paged && (x == 0) && (w == int16_t(WIDTH)) && (h < int16_t(HEIGHT)))
  {
    //Serial.println("paged");
    _startTransfer();
    for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(h) / 8; i++)
    {
      uint8_t data = bitmap[i];
      for (int16_t k = 0; k < 4; k++)
      {
        uint8_t data2 = (data & 0x80 ? 0x10 : 0x00) | (data & 0x40 ? 0x01 : 0x00);
        data <<= 2;
        _transfer(data2);
      }
    }
    _endTransfer();
    if (y + h == HEIGHT) // last page
    {
      //Serial.println("paged ended");
      _paged = false;
    }
  }
  else
  {
    _paged = false;
    int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
    x -= x % 8; // byte boundary
    w = wb * 8; // byte boundary
    if ((w <= 0) || (h <= 0)) return;
    _Init_Full();
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < int16_t(HEIGHT); i++)
    {
      for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
      {
        uint8_t data = 0xFF;
        if ((j >= x) && (j <= x + w) && (i >= y) && (i < y + h))
        {
          uint32_t idx = mirror_y ? (j - x) / 8 + uint32_t((h - 1 - (i - y))) * wb : (j - x) / 8 + uint32_t(i - y) * wb;
          if (pgm)
          {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
            data = pgm_read_byte(&bitmap[idx]);
#else
            data = bitmap[idx];
#endif
          }
          else
          {
            data = bitmap[idx];
          }
          if (invert) data = ~data;
        }
        for (int16_t k = 0; k < 4; k++)
        {
          uint8_t data2 = (data & 0x80 ? 0x10 : 0x00) | (data & 0x40 ? 0x01 : 0x00);
          data <<= 2;
          _transfer(data2);
        }
      }
    }
    _endTransfer();
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_730c_GDEY073D46::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (!black && !color) return;
  if (!color) return writeImage(black, x, y, w, h, invert, mirror_y, pgm);
  //Serial.print("writeImage("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if (_paged && (x == 0) && (w == int16_t(WIDTH)) && (h < int16_t(HEIGHT)))
  {
    //Serial.println("paged");
    _startTransfer();
    for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(h) / 8; i++)
    {
      uint8_t black_data = black[i];
      uint8_t color_data = color[i];
      for (int16_t k = 0; k < 4; k++)
      {
        uint8_t out_data = 0x00;
        for (int16_t l = 0; l < 2; l++)
        {
          out_data <<= 4;
          if (!(color_data & 0x80)) out_data |= 0x04;
          else out_data |= black_data & 0x80 ? 0x01 : 0x00;
          black_data <<= 1;
          color_data <<= 1;
        }
        _transfer(out_data);
      }
    }
    _endTransfer();
    if (y + h == HEIGHT) // last page
    {
      //Serial.println("paged ended");
      _paged = false;
    }
  }
  else
  {
    _paged = false;
    int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
    x -= x % 8; // byte boundary
    w = wb * 8; // byte boundary
    if ((w <= 0) || (h <= 0)) return;
    _Init_Full();
    _writeCommand(0x10);
    _startTransfer();
    for (int16_t i = 0; i < int16_t(HEIGHT); i++)
    {
      for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
      {
        uint8_t black_data = 0xFF, color_data = 0xFF;
        if ((j >= x) && (j < x + w) && (i >= y) && (i < y + h))
        {
          uint32_t idx = mirror_y ? (j - x) / 8 + uint32_t((h - 1 - (i - y))) * wb : (j - x) / 8 + uint32_t(i - y) * wb;
          if (pgm)
          {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
            black_data = pgm_read_byte(&black[idx]);
            color_data = pgm_read_byte(&color[idx]);
#else
            black_data = black[idx];
            color_data = color[idx];
#endif
          }
          else
          {
            black_data = black[idx];
            color_data = color[idx];
          }
          if (invert)
          {
            black_data = ~black_data;
            color_data = ~color_data;
          }
        }
        for (int16_t k = 0; k < 4; k++)
        {
          uint8_t out_data = 0x00;
          for (int16_t l = 0; l < 2; l++)
          {
            out_data <<= 4;
            if (!(color_data & 0x80)) out_data |= 0x04;
            else out_data |= black_data & 0x80 ? 0x01 : 0x00;
            black_data <<= 1;
            color_data <<= 1;
          }
          _transfer(out_data);
        }
      }
    }
    _endTransfer();
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_730c_GDEY073D46::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 7) / 8; // width bytes, bitmaps are padded
  x_part -= x_part % 8; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  x -= x % 8; // byte boundary
  w = 8 * ((w + 7) / 8); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  _Init_Full();
  _writeCommand(0x10);
  _startTransfer();
  for (int16_t i = 0; i < int16_t(HEIGHT); i++)
  {
    for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
    {
      uint8_t data = 0xFF;
      if ((j >= x1) && (j < x1 + w) && (i >= y1) && (i < y1 + h))
      {
        // use wb_bitmap, h_bitmap of bitmap for index!
        uint32_t idx = mirror_y ? (x_part + j - x1) / 8 + uint32_t((h_bitmap - 1 - (y_part + i - y1))) * wb_bitmap : (x_part + j - x1) / 8 + uint32_t(y_part + i - y1) * wb_bitmap;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&bitmap[idx]);
#else
          data = bitmap[idx];
#endif
        }
        else
        {
          data = bitmap[idx];
        }
        if (invert) data = ~data;
      }
      for (int16_t k = 0; k < 4; k++)
      {
        uint8_t data2 = (data & 0x80 ? 0x10 : 0x00) | (data & 0x40 ? 0x01 : 0x00);
        data <<= 2;
        _transfer(data2);
      }
    }
  }
  _endTransfer();
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_730c_GDEY073D46::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  //Serial.print("writeImagePart("); Serial.print(x_part); Serial.print(", "); Serial.print(y_part); Serial.print(", ");
  //Serial.print(w_bitmap); Serial.print(", "); Serial.print(h_bitmap); Serial.print(", ");
  //Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  if (!black && !color) return;
  if (!color) return writeImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 7) / 8; // width bytes, bitmaps are padded
  x_part -= x_part % 8; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  x -= x % 8; // byte boundary
  w = 8 * ((w + 7) / 8); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  _Init_Full();
  _writeCommand(0x10);
  _startTransfer();
  for (int16_t i = 0; i < int16_t(HEIGHT); i++)
  {
    for (int16_t j = 0; j < int16_t(WIDTH); j += 8)
    {
      uint8_t black_data = 0xFF, color_data = 0xFF;
      if ((j >= x1) && (j < x1 + w) && (i >= y1) && (i < y1 + h))
      {
        // use wb_bitmap, h_bitmap of bitmap for index!
        uint32_t idx = mirror_y ? (x_part + j - x1) / 8 + uint32_t((h_bitmap - 1 - (y_part + i - y1))) * wb_bitmap : (x_part + j - x1) / 8 + uint32_t(y_part + i - y1) * wb_bitmap;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          black_data = pgm_read_byte(&black[idx]);
          color_data = pgm_read_byte(&color[idx]);
#else
          black_data = black[idx];
          color_data = color[idx];
#endif
        }
        else
        {
          black_data = black[idx];
          color_data = color[idx];
        }
        if (invert)
        {
          black_data = ~black_data;
          color_data = ~color_data;
        }
      }
      for (int16_t k = 0; k < 4; k++)
      {
        uint8_t out_data = 0x00;
        for (int16_t l = 0; l < 2; l++)
        {
          out_data <<= 4;
          if (!(color_data & 0x80)) out_data |= 0x04;
          else out_data |= black_data & 0x80 ? 0x01 : 0x00;
          black_data <<= 1;
          color_data <<= 1;
        }
        _transfer(out_data);
      }
    }
  }
  _endTransfer();
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_730c_GDEY073D46::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    //Serial.print("writeNative("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
    //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
    delay(1); // yield() to avoid WDT on ESP8266 and ESP32
    if (_paged && (x == 0) && (w == int16_t(WIDTH)) && (h < int16_t(HEIGHT)))
    {
      //Serial.println("paged");
      _startTransfer();
      for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(h) / 2; i++)
      {
        uint8_t data = data1[i];
        _transfer(data);
      }
      _endTransfer();
      if (y + h == HEIGHT) // last page
      {
        //Serial.println("paged ended");
        _paged = false;
      }
    }
    else
    {
      _paged = false;
      int16_t wb = (w + 1) / 2; // width bytes, bitmaps are padded
      x -= x % 2; // byte boundary
      w = wb * 2; // byte boundary
      if ((w <= 0) || (h <= 0)) return;
      _Init_Full();
      _writeCommand(0x10);
      _startTransfer();
      for (int16_t i = 0; i < int16_t(HEIGHT); i++)
      {
        for (int16_t j = 0; j < int16_t(WIDTH); j += 2)
        {
          uint8_t data = 0x11;
          if (data1)
          {
            if ((j >= x) && (j < x + w) && (i >= y) && (i < y + h))
            {
              uint32_t idx = mirror_y ? (j - x) / 2 + uint32_t((h - 1 - (i - y))) * wb : (j - x) / 2 + uint32_t(i - y) * wb;
              if (pgm)
              {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
                data = pgm_read_byte(&data1[idx]);
#else
                data = data1[idx];
#endif
              }
              else
              {
                data = data1[idx];
              }
              if (invert) data = ~data;
            }
          }
          _transfer(data);
        }
      }
      _endTransfer();
    }
    delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  }
}

void GxEPD2_730c_GDEY073D46::writeNativePart(const uint8_t* data1, const uint8_t* data2, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  //Serial.print("writeNativePart("); Serial.print(x_part); Serial.print(", "); Serial.print(y_part); Serial.print(", ");
  //Serial.print(w_bitmap); Serial.print(", "); Serial.print(h_bitmap); Serial.print(", ");
  //Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
  //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
  if (!data1) return;
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 1) / 2; // width bytes, bitmaps are padded
  x_part -= x_part % 2; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  x -= x % 2; // byte boundary
  w = 2 * ((w + 1) / 2); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  _Init_Full();
  _writeCommand(0x10);
  _startTransfer();
  for (int16_t i = 0; i < int16_t(HEIGHT); i++)
  {
    for (int16_t j = 0; j < int16_t(WIDTH); j += 2)
    {
      uint8_t data = 0x11;
      if ((j >= x1) && (j < x1 + w) && (i >= y1) && (i < y1 + h))
      {
        // use wb_bitmap, h_bitmap of bitmap for index!
        uint32_t idx = mirror_y ? (x_part + j - x1) / 2 + uint32_t((h_bitmap - 1 - (y_part + i - y1))) * wb_bitmap : (x_part + j - x1) / 2 + uint32_t(y_part + i - y1) * wb_bitmap;
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&data1[idx]);
#else
          data = data1[idx];
#endif
        }
        else
        {
          data = data1[idx];
        }
        if (invert) data = ~data;
      }
      _transfer(data);
    }
  }
  _endTransfer();
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_730c_GDEY073D46::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_730c_GDEY073D46::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_730c_GDEY073D46::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(black, color, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_730c_GDEY073D46::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(black, color, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_730c_GDEY073D46::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeNative(data1, data2, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_730c_GDEY073D46::refresh(bool partial_update_mode)
{
  if (partial_update_mode) refresh(0, 0, WIDTH, HEIGHT);
  else _Update_Full();
}

void GxEPD2_730c_GDEY073D46::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
  _Update_Part();
}

void GxEPD2_730c_GDEY073D46::powerOff()
{
  _PowerOff();
}

void GxEPD2_730c_GDEY073D46::hibernate()
{
  _PowerOff();
  if (_rst >= 0)
  {
    _writeCommand(0x07); // deep sleep
    _writeData(0xA5);    // control code
    _hibernating = true;
  }
}

void GxEPD2_730c_GDEY073D46::setPaged()
{
  _paged = true;
  _Init_Full();
  _writeCommand(0x10);
}

void GxEPD2_730c_GDEY073D46::_PowerOn()
{
  if (!_power_is_on)
  {
    _writeCommand(0x04);
    _waitWhileBusy("_PowerOn", power_on_time);
  }
  _power_is_on = true;
}

void GxEPD2_730c_GDEY073D46::_PowerOff()
{
  if (_power_is_on)
  {
    _writeCommand(0x02);
    _writeData(0x00);
    _waitWhileBusy("_PowerOff", power_off_time);
  }
  _power_is_on = false;
  _using_partial_mode = false;
}

void GxEPD2_730c_GDEY073D46::_InitDisplay()
{
  if (_hibernating) _reset();
  _writeCommand(0xAA); // CMDH
  _writeData(0x49);
  _writeData(0x55);
  _writeData(0x20);
  _writeData(0x08);
  _writeData(0x09);
  _writeData(0x18);
  _writeCommand(0x01); // PWRR
  _writeData(0x3F);
  _writeData(0x00);
  _writeData(0x32);
  _writeData(0x2A);
  _writeData(0x0E);
  _writeData(0x2A);
  _writeCommand(0x00); // PSR
  _writeData(0x5F);
  _writeData(0x69);
  _writeCommand(0x03); // POFS
  _writeData(0x00);
  _writeData(0x54);
  _writeData(0x00);
  _writeData(0x44);
  _writeCommand(0x05); // BTST1
  _writeData(0x40);
  _writeData(0x1F);
  _writeData(0x1F);
  _writeData(0x2C);
  _writeCommand(0x06); // BTST2
  _writeData(0x6F);
  _writeData(0x1F);
  _writeData(0x16);
  _writeData(0x25);
  _writeCommand(0x08); // BTST3
  _writeData(0x6F);
  _writeData(0x1F);
  _writeData(0x1F);
  _writeData(0x22);
  _writeCommand(0x13); // IPC
  _writeData(0x00);
  _writeData(0x04);
  _writeCommand(0x30); // PLL
  _writeData(0x02);
  _writeCommand(0x41); // TSE
  _writeData(0x00);
  _writeCommand(0x50); // CDI
  _writeData(0x3F);
  _writeCommand(0x60); // TCON
  _writeData(0x02);
  _writeData(0x00);
  _writeCommand(0x61); // TRES
  _writeData(0x03);
  _writeData(0x20);
  _writeData(0x01);
  _writeData(0xE0);
  _writeCommand(0x82); // VDCS
  _writeData(0x1E);
  _writeCommand(0x84); // T_VDCS
  _writeData(0x00);
  _writeCommand(0x86); // AGID
  _writeData(0x00);
  _writeCommand(0xE3); // PWS
  _writeData(0x2F);
  _writeCommand(0xE0); // CCSET
  _writeData(0x00);
  _writeCommand(0xE6); // TSSET
  _writeData(0x00);
}

void GxEPD2_730c_GDEY073D46::_Init_Full()
{
  _InitDisplay();
  _PowerOn();
}

void GxEPD2_730c_GDEY073D46::_Init_Part()
{
  _InitDisplay();
  _PowerOn();
}

void GxEPD2_730c_GDEY073D46::_Update_Full()
{
  _writeCommand(0x12); // Display Refresh
  _writeData(0x00);
  delay(1);
  _waitWhileBusy("_Update_Full", full_refresh_time);
}

void GxEPD2_730c_GDEY073D46::_Update_Part()
{
  _writeCommand(0x12); // Display Refresh
  _writeData(0x00);
  delay(1);
  _waitWhileBusy("_Update_Part", partial_refresh_time);
}

uint8_t GxEPD2_730c_GDEY073D46::_colorOfDemoBitmap(uint8_t from)
{
  switch (from)
  {
    case 0xFF: return(0x01); // white;
    case 0xFC: return(0x05); // yellow;
    case 0xF1: return(0x06); // orange;
    case 0xE5: return(0x04); // red;
    case 0x4B: return(0x03); // blue;
    case 0x39: return(0x02); // green;
    case 0x00: return(0x00); // black;
    default: return(0x01); // white;
  }
}

void GxEPD2_730c_GDEY073D46::writeDemoBitmap(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, int16_t mode, bool mirror_y, bool pgm)
{
  (void) mode;
  if (data1)
  {
    //Serial.print("writeNative("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", ");
    //Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(")");
    delay(1); // yield() to avoid WDT on ESP8266 and ESP32
    {
      _paged = false;
      if ((w <= 0) || (h <= 0)) return;
      _Init_Full();
      _writeCommand(0x10);
      _startTransfer();
      for (int16_t i = 0; i < int16_t(HEIGHT); i++)
      {
        for (int16_t j = 0; j < int16_t(WIDTH); j += 2)
        {
          uint8_t data = 0x11;
          if (data1)
          {
            if ((j >= x) && (j < x + w) && (i >= y) && (i < y + h))
            {
              uint32_t idx = mirror_y ? (j - x) + uint32_t((h - 1 - (i - y))) * w : (j - x) + uint32_t(i - y) * w;
              if (pgm)
              {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
                data = _colorOfDemoBitmap(pgm_read_byte(&data1[idx++])) << 4;
                data |= _colorOfDemoBitmap(pgm_read_byte(&data1[idx]));
#else
                data = _colorOfDemoBitmap(data1[idx++]) << 4;
                data |= _colorOfDemoBitmap(data1[idx]);
#endif
              }
              else
              {
                data = _colorOfDemoBitmap(data1[idx++]) << 4;
                data |= _colorOfDemoBitmap(data1[idx]);
              }
            }
          }
          _transfer(data);
        }
      }
      _endTransfer();
    }
    delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  }
}

void GxEPD2_730c_GDEY073D46::drawDemoBitmap(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, int16_t mode, bool mirror_y, bool pgm)
{
  writeDemoBitmap(data1, data2, x, y, w, h, mode, mirror_y, pgm);
  refresh(x, y, w, h);
}
