/*  ---------------------------------------------------------------------------
    File: Led_Strip.h
    Author(s):  Allen Kempe <allenck@windstream.net>
    Date Created: 05/14/2019
    Last modified: 05/14/2019
    Description: 
    This library can drive led strips through the RMT module on the ESP32.
    GITHUB: https://github.com/allenck/ESP32_led_strip/

    Modified for OpenDTU integration
    Author(s): Florent Carlier <tifaifai.maupiti@gmail.com>
    Date Created: 07/25/2023
    Last modified: 07/25/2023
    Description:
    Coding style of OpenDTU with different effect on the ESP32-S3 board 
    ------------------------------------------------------------------------ */

#ifndef STRIP_H
#define STRIP_H

#include <driver/rmt.h>
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <stddef.h>
#include <time.h>
#include <TimeoutHelper.h>


// The order of primary colors in the NeoPixel data stream can vary among
// device types, manufacturers and even different revisions of the same
// item.  The third parameter to the Adafruit_NeoPixel constructor encodes
// the per-pixel byte offsets of the red, green and blue primaries (plus
// white, if present) in the data stream -- the following #defines provide
// an easier-to-use named version for each permutation. e.g. NEO_GRB
// indicates a NeoPixel-compatible device expecting three bytes per pixel,
// with the first byte transmitted containing the green value, second
// containing red and third containing blue. The in-memory representation
// of a chain of NeoPixels is the same as the data-stream order; no
// re-ordering of bytes is required when issuing data to the chain.
// Most of these values won't exist in real-world devices, but it's done
// this way so we're ready for it (also, if using the WS2811 driver IC,
// one might have their pixels set up in any weird permutation).

// Bits 5,4 of this value are the offset (0-3) from the first byte of a
// pixel to the location of the red color byte.  Bits 3,2 are the green
// offset and 1,0 are the blue offset.  If it is an RGBW-type device
// (supporting a white primary in addition to R,G,B), bits 7,6 are the
// offset to the white byte...otherwise, bits 7,6 are set to the same value
// as 5,4 (red) to indicate an RGB (not RGBW) device.
// i.e. binary representation:
// 0bWWRRGGBB for RGBW devices
// 0bRRRRGGBB for RGB

// RGB NeoPixel permutations; white and red offsets are always same
// Offset:         W        R        G        B
#define NEO_RGB  ((0<<6) | (0<<4) | (1<<2) | (2)) ///< Transmit as R,G,B
#define NEO_RBG  ((0<<6) | (0<<4) | (2<<2) | (1)) ///< Transmit as R,B,G
#define NEO_GRB  ((1<<6) | (1<<4) | (0<<2) | (2)) ///< Transmit as G,R,B
#define NEO_GBR  ((2<<6) | (2<<4) | (0<<2) | (1)) ///< Transmit as G,B,R
#define NEO_BRG  ((1<<6) | (1<<4) | (2<<2) | (0)) ///< Transmit as B,R,G
#define NEO_BGR  ((2<<6) | (2<<4) | (1<<2) | (0)) ///< Transmit as B,G,R

// RGBW NeoPixel permutations; all 4 offsets are distinct
// Offset:         W          R          G          B
#define NEO_WRGB ((0<<6) | (1<<4) | (2<<2) | (3)) ///< Transmit as W,R,G,B
#define NEO_WRBG ((0<<6) | (1<<4) | (3<<2) | (2)) ///< Transmit as W,R,B,G
#define NEO_WGRB ((0<<6) | (2<<4) | (1<<2) | (3)) ///< Transmit as W,G,R,B
#define NEO_WGBR ((0<<6) | (3<<4) | (1<<2) | (2)) ///< Transmit as W,G,B,R
#define NEO_WBRG ((0<<6) | (2<<4) | (3<<2) | (1)) ///< Transmit as W,B,R,G
#define NEO_WBGR ((0<<6) | (3<<4) | (2<<2) | (1)) ///< Transmit as W,B,G,R

#define NEO_RWGB ((1<<6) | (0<<4) | (2<<2) | (3)) ///< Transmit as R,W,G,B
#define NEO_RWBG ((1<<6) | (0<<4) | (3<<2) | (2)) ///< Transmit as R,W,B,G
#define NEO_RGWB ((2<<6) | (0<<4) | (1<<2) | (3)) ///< Transmit as R,G,W,B
#define NEO_RGBW ((3<<6) | (0<<4) | (1<<2) | (2)) ///< Transmit as R,G,B,W
#define NEO_RBWG ((2<<6) | (0<<4) | (3<<2) | (1)) ///< Transmit as R,B,W,G
#define NEO_RBGW ((3<<6) | (0<<4) | (2<<2) | (1)) ///< Transmit as R,B,G,W

#define NEO_GWRB ((1<<6) | (2<<4) | (0<<2) | (3)) ///< Transmit as G,W,R,B
#define NEO_GWBR ((1<<6) | (3<<4) | (0<<2) | (2)) ///< Transmit as G,W,B,R
#define NEO_GRWB ((2<<6) | (1<<4) | (0<<2) | (3)) ///< Transmit as G,R,W,B
#define NEO_GRBW ((3<<6) | (1<<4) | (0<<2) | (2)) ///< Transmit as G,R,B,W
#define NEO_GBWR ((2<<6) | (3<<4) | (0<<2) | (1)) ///< Transmit as G,B,W,R
#define NEO_GBRW ((3<<6) | (2<<4) | (0<<2) | (1)) ///< Transmit as G,B,R,W

#define NEO_BWRG ((1<<6) | (2<<4) | (3<<2) | (0)) ///< Transmit as B,W,R,G
#define NEO_BWGR ((1<<6) | (3<<4) | (2<<2) | (0)) ///< Transmit as B,W,G,R
#define NEO_BRWG ((2<<6) | (1<<4) | (3<<2) | (0)) ///< Transmit as B,R,W,G
#define NEO_BRGW ((3<<6) | (1<<4) | (2<<2) | (0)) ///< Transmit as B,R,G,W
#define NEO_BGWR ((2<<6) | (3<<4) | (1<<2) | (0)) ///< Transmit as B,G,W,R
#define NEO_BGRW ((3<<6) | (2<<4) | (1<<2) | (0)) ///< Transmit as B,G,R,W

#define NEO_KHZ800 0

/****************************
        WS2812 Timing
 ****************************/
#define LED_STRIP_RMT_TICKS_BIT_1_HIGH_WS2812 9 // 900ns (900ns +/- 150ns per datasheet)
#define LED_STRIP_RMT_TICKS_BIT_1_LOW_WS2812  3 // 300ns (350ns +/- 150ns per datasheet)
#define LED_STRIP_RMT_TICKS_BIT_0_HIGH_WS2812 3 // 300ns (350ns +/- 150ns per datasheet)
#define LED_STRIP_RMT_TICKS_BIT_0_LOW_WS2812  9 // 900ns (900ns +/- 150ns per datasheet)
// These two tables are declared outside the Adafruit_NeoPixel class
// because some boards may require oldschool compilers that don't
// handle the C++11 constexpr keyword.

/* A PROGMEM (flash mem) table containing 8-bit unsigned sine wave (0-255).
   Copy & paste this snippet into a Python REPL to regenerate:
import math
for x in range(256):
    print("{:3},".format(int((math.sin(x/128.0*math.pi)+1.0)*127.5+0.5))),
    if x&15 == 15: print
*/
static const uint8_t /*PROGMEM*/ _NeoPixelSineTable[256] = {
  128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,
  176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,
  218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,
  245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,
  255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,
  245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220,
  218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,
  176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,
  128,124,121,118,115,112,109,106,103,100, 97, 93, 90, 88, 85, 82,
   79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40,
   37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 17, 15, 14, 12, 11,
   10,  9,  7,  6,  5,  5,  4,  3,  2,  2,  1,  1,  1,  0,  0,  0,
    0,  0,  0,  0,  1,  1,  1,  2,  2,  3,  4,  5,  5,  6,  7,  9,
   10, 11, 12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35,
   37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76,
   79, 82, 85, 88, 90, 93, 97,100,103,106,109,112,115,118,121,124};

/* Similar to above, but for an 8-bit gamma-correction table.
   Copy & paste this snippet into a Python REPL to regenerate:
import math
gamma=2.6
for x in range(256):
    print("{:3},".format(int(math.pow((x)/255.0,gamma)*255.0+0.5))),
    if x&15 == 15: print
*/
static const uint8_t /*PROGMEM*/ _NeoPixelGammaTable[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,
    3,  3,  4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  7,
    7,  7,  8,  8,  8,  9,  9,  9, 10, 10, 10, 11, 11, 11, 12, 12,
   13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20,
   20, 21, 21, 22, 22, 23, 24, 24, 25, 25, 26, 27, 27, 28, 29, 29,
   30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 38, 38, 39, 40, 41, 42,
   42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
   58, 59, 60, 61, 62, 63, 64, 65, 66, 68, 69, 70, 71, 72, 73, 75,
   76, 77, 78, 80, 81, 82, 84, 85, 86, 88, 89, 90, 92, 93, 94, 96,
   97, 99,100,102,103,105,106,108,109,111,112,114,115,117,119,120,
  122,124,125,127,129,130,132,134,136,137,139,141,143,145,146,148,
  150,152,154,156,158,160,162,164,166,168,170,172,174,176,178,180,
  182,184,186,188,191,193,195,197,199,202,204,206,209,211,213,215,
  218,220,223,225,227,230,232,235,237,240,242,245,247,250,252,255};

//clock_t millis();
//inline void delay(uint32_t d) { vTaskDelay(d / portTICK_PERIOD_MS);}

// Function pointer for generating waveforms based on different LED drivers
typedef void (*led_fill_rmt_items_fn)(uint32_t *led_strip_buf, rmt_item32_t *rmt_items, uint32_t led_strip_length, uint8_t wOffset, uint8_t rOffset,uint8_t gOffset,uint8_t bOffset);


typedef uint8_t  NeoPixelType; ///< 3rd arg to Adafruit_NeoPixel constructor
class Color
{
 public:
   Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t white =0);
	 Color(uint32_t c);
	 uint32_t value();
	 uint8_t red();
	 uint8_t green();
	 uint8_t blue();
	 uint8_t white();
	private:
 	 uint32_t val = 0;
   
};

#define LEDSTRIP_UPDATE_INTERVAL 2000

class LedStripClass
{
public:
    LedStripClass();
    ~LedStripClass();

	bool init(); ///uint16_t n, uint8_t p=17, NeoPixelType t=NEO_GRB+NEO_KHZ800, uint8_t ch = 0);
    void loop();
    void loop_example();

	bool show();
	bool clearOnShow();
	void setClearOnShow(bool);
	/**
	* Sets the pixel at pixel_num to color.
	*/
	bool setPixelColor(uint32_t pixel_num, uint32_t color);
    uint16_t numPixels() {return _numPixels;}
	bool clear();
	bool getPixelColor(uint32_t pixel_num, uint32_t color);
    uint8_t gamma8(uint8_t x);
	void fill(uint32_t c, uint16_t first, uint16_t count);
	uint32_t ColorHSV(uint16_t hue, uint8_t sat, uint8_t val);
	uint32_t gamma32(uint32_t x);


private:
 
	uint32_t* buf1 = NULL;
    uint32_t* buf2 = NULL;
	bool showingBuf1 = false;
	uint16_t _numPixels = 0;
	uint8_t gpio = 0;
	NeoPixelType neoPixelType = 0;
	uint8_t rmtChannel = 0;
    SemaphoreHandle_t access_semaphore;
	uint8_t rOffset;    ///< Red index within each 3- or 4-byte pixel
	uint8_t gOffset;    ///< Index of green byte
	uint8_t bOffset;    ///< Index of blue byte
	uint8_t wOffset;    ///< Index of white (==rOffset if no white)
	bool hasWhite = false;
    bool bClearOnShow=false;
    void* led_strip_task_handle = NULL;
	 
	bool init_rmt();
    static void led_strip_task(void *arg);
	static void led_strip_fill_rmt_items_ws2812(uint32_t *led_strip_buf, rmt_item32_t *rmt_items, uint32_t led_strip_length, uint8_t wOffset, uint8_t rOffset,uint8_t gOffset,uint8_t bOffset);
	 
    static inline void led_strip_fill_item_level(rmt_item32_t* item, int high_ticks, int low_ticks)
	{
		  item->level0 = 1;
		  item->duration0 = high_ticks;
		  item->level1 = 0;
		  item->duration1 = low_ticks;
	}
	static inline void led_strip_rmt_bit_0_ws2812(rmt_item32_t* item)
	{
		  led_strip_fill_item_level(item, LED_STRIP_RMT_TICKS_BIT_0_HIGH_WS2812, LED_STRIP_RMT_TICKS_BIT_0_LOW_WS2812);
	}
	static inline void led_strip_rmt_bit_1_ws2812(rmt_item32_t* item)
	{
          led_strip_fill_item_level(item, LED_STRIP_RMT_TICKS_BIT_1_HIGH_WS2812, LED_STRIP_RMT_TICKS_BIT_1_LOW_WS2812);
    }

    void colorWipe(uint32_t color, int wait);
    void whiteOverRainbow(int whiteSpeed, int whiteLength);
    void pulseWhite(uint8_t wait);
    void rainbowFade2White(int wait, int rainbowLoops, int whiteLoops);
    void theaterChase(uint32_t color, int wait);
    void rainbow(int wait);
    void theaterChaseRainbow(int wait);

    enum class LedRGBState_t {
        OnN,
        OffN,
        BlinkN,
        OnI,
        OffI,
        BlinkI
    };

    LedRGBState_t _ledRGBState[2];
    TimeoutHelper _updateRGBTimeout;
    uint8_t _ledRGBActive = 0;

};

extern LedStripClass LEDStrip;

#endif