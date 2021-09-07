// Set Arduino Board to "Adafruit ESP32 MagTag 2.9"" - all defaults...


/***************************************************
  This is our Bitmap drawing example for the Adafruit EPD Breakout and Shield
  ----> http://www.adafruit.com/products/3625
  Check out the links above for our tutorials and wiring diagrams
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!
  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution


  THE BMP IMAGE YOU ARE USING MUST BE THE SAME DIMENSIONS AS YOUR EPD DISPLAY
  For the 1.54" EPD, this is 152x152 pixels
  For the 2.13" EPD, this is 104x212 pixels
 ****************************************************/

#include <SPI.h>
#include <Wire.h>
#include "time.h"
//#include <GxEPD2_BW.h>
//#include <GxEPD2_3C.h>
#include "Adafruit_ThinkInk.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_SSD1306.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "Adafruit_EPD.h"    // Hardware-specific library
#include "wifi_password.h"


// EPD display and SD card will share the hardware SPI interface.
// Hardware SPI pins are specific to the Arduino board type and
// cannot be remapped to alternate pins.  For Arduino Uno,
// Duemilanove, etc., pin 11 = MOSI, pin 12 = MISO, pin 13 = SCK.

#if defined(ESP8266)

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#define EPD_CS     0
#define EPD_DC     15
#define SRAM_CS    16
#define EPD_RESET  -1  // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY   -1  // can set to -1 to not use a pin (will wait a fixed delay)
#define SD_CS      2
#define CONNECTED
#define VREF_PIN   35

#elif defined( ESP32 )

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#define EPD_CS     8
#define EPD_DC     7
#define SRAM_CS    -1
#define EPD_RESET  6   // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY   5   // can set to -1 to not use a pin (will wait a fixed delay)
#define SD_CS      -1
#define CONNECTED
#define VREF_PIN   35

#else

#define EPD_CS     9
#define EPD_DC     10
#define SRAM_CS    6
#define EPD_RESET  -1  // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY   -1  // can set to -1 to not use a pin (will wait a fixed delay)
#define SD_CS      5
#define VREF_PIN   -1

#endif


#define kPadding   3
#define BATTERY_VID_THRESHOLD 50   // percent
#define LOW_BATTERY_THRESHOLD 10   // percent

/* Uncomment the following line if you are using 1.54" tricolor EPD */
//Adafruit_IL0373 display(152, 152 ,EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

/* Uncomment the following line if you are using 2.15" tricolor EPD */
//Adafruit_IL0373 display(212, 104 , EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

/* Uncomment the following line if you are using 2.7" tricolor EPD */
//Adafruit_IL91874 display(264, 176 ,EPD_DC, EPD_RESET, EPD_CS, SRAM_CS);
//Adafruit_SSD1306 oled = Adafruit_SSD1306( 128, 32, &Wire );

// 2.9" Grayscale Featherwing or Breakout:
ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);


U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;  // Select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall


// OLED FeatherWing buttons map to different pins depending on board:
#if defined(ESP8266)
#define BUTTON_A  0
#define BUTTON_B 16
#define BUTTON_C  2
#elif defined(ESP32)
#define BUTTON_A 15
#define BUTTON_B 32
#define BUTTON_C 14
#elif defined(ARDUINO_STM32_FEATHER)
#define BUTTON_A PA15
#define BUTTON_B PC7
#define BUTTON_C PC5
#elif defined(TEENSYDUINO)
#define BUTTON_A  4
#define BUTTON_B  3
#define BUTTON_C  8
#elif defined(ARDUINO_FEATHER52832)
#define BUTTON_A 31
#define BUTTON_B 30
#define BUTTON_C 27
#else // 32u4, M0, M4, nrf52840 and 328p
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5
#endif

// hack
#ifndef GxEPD_BLACK
#define GxEPD_BLACK EPD_BLACK
#endif 

/*static const uint8_t PROGMEM s_folabs_logo_inverted[] =
  {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0x01, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x66, 0x7f, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xe7, 0x9f,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xf3, 0xe7, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xe3, 0xef, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0xc3, 0xef,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xef, 0x81, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xee, 0x01, 0xc0, 0xf7, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc1, 0x46, 0x77,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xcc, 0xc9, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xee, 0x0d, 0x01, 0xbf, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0x03, 0x67,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xf7, 0x83, 0xef, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xc7, 0xdf, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf3, 0xcf, 0xbf,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xfc, 0xef, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xcf, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xef, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
  };
*/

static const uint8_t PROGMEM s_folabs_logo[] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x20, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x10, 0x80, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x30, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x08, 0x38, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x7c, 0x10, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xfc, 0x98,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x11, 0xf2, 0xfe, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x36, 0xf8, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0xb9, 0x88,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x11, 0xfe, 0x3f, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x7e, 0x08, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x3c, 0x10,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x08, 0x1c, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x18, 0x20, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x18, 0x60,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x03, 0x99, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x08, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


enum alignmentType {LEFT, RIGHT, CENTER};

static const char*    ssid     = STASSID;
static const char*    password = STAPSK;
static const char*    host     = "http://10.0.1.11/wx.html"; // this is aprs.local (which has a fixed IP address)

static const char* compass[] = { "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW" };

static const char* Timezone           = "PST8PDT,M3.2.0,M11.1.0";
static const char* ntpServer          = "pool.ntp.org";
static int         gmtOffset_sec      = -25200; // GMT Offset is 0, for US (-5Hrs) is typically -18000, AU is typically (+8hrs) 28800
static int         daylightOffset_sec = 3600;   // In the US/UK DST is +1hr or 3600-secs, other countries may use 2hrs 7200 or 30-mins 1800 or 5.5hrs 19800 Ahead of GMT use + offset behind - offset

static long SleepDuration = 5;  // Sleep time in minutes
static int  WakeupTime    = 7;  // Don't wakeup until after 07:00 to save battery power
static int  SleepTime     = 23; // Sleep after (23+1) 00:00 to save battery power
static int  s_wifi_signal = 0;

static String  time_str;
static int     CurrentHour = 0, CurrentMin = 0, CurrentSec = 0;
static long    StartTime = 0;

static int tempOffsetX = 85;

static const int kMaxGraphPoints = 80;
static const int kGraphHeight    = 18;

static const int kMaxAQIGraphPoints = 160;
static const int kMaxAQIDataPoints  = kMaxAQIGraphPoints * 2; // this gets us about 24 hours of data (slightly more 26.6)
static const int kAQIGraphHeight    = 20;


RTC_DATA_ATTR size_t  rtc_graph_count = 0;
RTC_DATA_ATTR float   rtc_graph[kMaxGraphPoints] = {0};

RTC_DATA_ATTR int16_t rtc_aqi_pm24 = 0;     // 24 hr average

typedef struct
{
  const char* category;
  int   loAQI;
  int   hiAQI;
  float loBreak;
  float hiBreak;
} break_element_t;

static const char* kCatGood          = "Good";
static const char* kCatModerate      = "Moderate";
static const char* kCatUnhealthySens = "Kinda Unhealthy";
static const char* kCatUnhealthy     = "Unhealthy";
static const char* kCatVeryUnhealthy = "Very Unhealthy";
static const char* kCatHazardous     = "Hazardous";
static const char* kCatHazardous2    = "Really Hazardous";
static const char* kCatHazardous3    = "HAZARDOUS!!";

// obtained from here: https://aqs.epa.gov/aqsweb/documents/codetables/aqi_breakpoints.csv
static const break_element_t s_break_list_pm100[] = 
{
  { kCatGood,            0,  50,    0.0,  54.0   },
  { kCatModerate,       51, 100,   55.0, 154.0   },
  { kCatUnhealthySens, 101, 150,  155.0, 254.0   },
  { kCatUnhealthy,     151, 200,  255.0, 354.0   },
  { kCatVeryUnhealthy, 201, 300,  355.0, 424.0   },
  { kCatHazardous,     301, 400,  425.0, 504.0   },
  { kCatHazardous2,    401, 500,  505.0, 604.0   },
  { kCatHazardous3,    501, 999,  605.0, 99999.0 }
};


static const break_element_t s_break_list_pm25[] = 
{
  { kCatGood,            0,  50,    0.0, 12.0    },
  { kCatModerate,       51, 100,   12.1, 35.4    },
  { kCatUnhealthySens, 101, 150,   35.5, 55.4    },
  { kCatUnhealthy,     151, 200,   55.5, 150.4   },
  { kCatVeryUnhealthy, 201, 300,  150.5, 250.4   },
  { kCatHazardous,     301, 400,  250.5, 350.4   },
  { kCatHazardous2,    401, 500,  350.5, 500.4   },
  { kCatHazardous3,    501, 999,  500.5, 99999.9 }
};

static const int kNumBreaklistElements = 8;





void drawString(int x, int y, String text, alignmentType alignment) 
{
  int16_t  x1, y1; //the bounds of x,y and w and h of the variable 'text' in pixels.
  uint16_t w, h;
  display.setTextWrap(false);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (alignment == RIGHT)  x = x - w;
  if (alignment == CENTER) x = x - w / 2;
  u8g2Fonts.setCursor(x, y + h);
  u8g2Fonts.print(text);
}


uint16_t getTextWidth( String text ) 
{
  int16_t  x1, y1; 
  uint16_t w, h;
  display.setTextWrap(false);
  display.getTextBounds( text, 0, 0, &x1, &y1, &w, &h );
  return w;
}


void draw_single_raindrop(int x, int y, int scale) 
{
  display.fillCircle(x, y, scale / 2, EPD_BLACK);
  display.fillTriangle(x - scale / 2, y, x, y - scale * 1.2, x + scale / 2, y , EPD_BLACK);
}


void draw_pressure_icon(int x, int y, int scale) 
{
  display.fillTriangle( x - scale / 2, y,   x, y + scale * 1.2,    x + scale / 2, y , EPD_BLACK);
}

boolean SetupTime() 
{
  configTime( gmtOffset_sec, daylightOffset_sec, ntpServer, "time.nist.gov"); //(gmtOffset_sec, daylightOffset_sec, ntpServer)
  setenv("TZ", Timezone, 1);  //setenv()adds the "TZ" variable to the environment with a value TimeZone, only used if set to 1, 0 means no change
  tzset(); // Set the TZ environment variable
  delay(100);
  bool TimeStatus = UpdateLocalTime();
  return TimeStatus;
}


boolean UpdateLocalTime() 
{
  struct tm timeinfo;
  char   time_output[30], day_output[30], update_time[30];
  while( !getLocalTime( &timeinfo, 5000 ) ) 
  { 
    // Wait for 5-sec for time to synchronise
    Serial.println( "Failed to obtain time" );
    return false;
  }
  
  CurrentHour = timeinfo.tm_hour;
  CurrentMin  = timeinfo.tm_min;
  CurrentSec  = timeinfo.tm_sec;
  Serial.println( &timeinfo, "%a %b %d %Y   %H:%M:%S" );      // Displays: Saturday, June 24 2017 14:05:49
  
  //See http://www.cplusplus.com/reference/ctime/strftime/
  strftime(update_time, sizeof(update_time), "%r", &timeinfo);        // Creates: '@ 02:05:49pm'
  sprintf(time_output, "%s", update_time);

  time_str = time_output;
  return true;
}


void Blink(byte PIN, int DELAY_MS, byte loops)
{
  for (byte i = 0; i < loops; i++)
  {
    digitalWrite(PIN, LOW);
    delay(DELAY_MS);
    digitalWrite(PIN, HIGH);
    delay(DELAY_MS);
  }
}


void BeginSleep()
{
  // turn off the led if on
  pinMode( LED_BUILTIN, INPUT );
  digitalWrite( LED_BUILTIN, HIGH );

  Serial.println( "Starting deep-sleep period..." );
//  display.powerDown();
  
  // put microcontroller to sleep, wake up after specified time
  ESP.deepSleep( SleepDuration * 60 * 1e6 );
}


void setup(void)
{
  StartTime = millis();
  Serial.begin( 115200 );
//  while (!Serial);   // time to get serial running

  pinMode( LED_BUILTIN, OUTPUT );

#ifdef USE_SD_CARD
  Serial.print("Initializing SD card...");
  if( !SD.begin( SD_CS ) )
      Serial.println("SD Card Init failed!");
#endif

  u8g2Fonts.begin(display);                  // connect u8g2 procedures to Adafruit GFX
  u8g2Fonts.setFontMode(1);                  // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(0);             // left to right (this is default)
  u8g2Fonts.setForegroundColor(EPD_BLACK); // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(EPD_WHITE); // apply Adafruit GFX color

#ifdef CONNECTED
  StartWiFi(); // this routine keeps on trying in poor wifi conditions...
//  SetupTime();

  DisplayWeather();
  BeginSleep();
#endif
}




uint8_t StartWiFi() 
{
    Serial.print("\nConnecting to: "); Serial.println( String( ssid ) );
    WiFi.disconnect();
    WiFi.mode( WIFI_STA );
    WiFi.setAutoConnect( true );
    WiFi.setAutoReconnect( true );
    WiFi.begin( ssid, password );
    
    unsigned long start = millis();
    uint8_t       connectionStatus;
    bool          AttemptConnection = true;
    
    while( AttemptConnection ) 
    {
        connectionStatus = WiFi.status();
        if( millis() > start + 15000 ) 
        { 
            // Wait 15-secs maximum
            AttemptConnection = false;
        }
        
        if( connectionStatus == WL_CONNECTED || connectionStatus == WL_CONNECT_FAILED ) 
        {
            AttemptConnection = false;
        }
        
        delay( 50 );
        Serial.print( "." );
    }
    
    if( connectionStatus == WL_CONNECTED ) 
    {
      s_wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
      Serial.println( "WiFi connected: " + WiFi.localIP().toString() );
    }
    else 
       Serial.println("WiFi connection *** FAILED ***");
       
    return connectionStatus;
}


//#########################################################################################
void StopWiFi() 
{
  WiFi.disconnect();
  WiFi.mode( WIFI_OFF );
}


void DisplayWeather()
{
  WiFiClient client;
  HTTPClient http;

  Serial.print("[HTTP] begin...\n");
  if( http.begin( client, host ) ) 
  {  
    // HTTP
    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();

        setup_epd( payload.c_str() );
//        Serial.println(payload);
        Blink( LED_BUILTIN, 100, 1 );
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.printf("[HTTP} Unable to connect\n");
  }
}


void setup_epd( const char* wxText )
{
//  UpdateLocalTime();
  Serial.println("EPD begun");

  struct tm timeinfo = {};
  float tempF = 0;
  int   humidity = 0;
  float pressure = 0;
  float windDirection = 0;
  float windSpeed = 0;
  float gustSpeed = 0;

  int pm10_standard  = 0;       // Standard PM1.0
  int pm25_standard  = 0;       // Standard PM2.5
  int pm100_standard = 0;       // Standard PM10.0
  int pm10_env  = 0;            // Environmental PM1.0
  int pm25_env  = 0;            // Environmental PM2.5
  int pm100_env = 0;            // Environmental PM10.0
  int particles_03um  = 0;      // 0.3um Particle Count
  int particles_05um  = 0;      // 0.5um Particle Count
  int particles_10um  = 0;      // 1.0um Particle Count
  int particles_25um  = 0;      // 2.5um Particle Count
  int particles_50um  = 0;      // 5.0um Particle Count
  int particles_100um = 0;      // 10.0um Particle Count
  int pm25aqi = 0;              // 24 hours average of PM2.5 data

  int scanned = sscanf( wxText, "%02d:%02d:%02d, %g, %d, %g, %g, %g, %g, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n", &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec, &tempF, &humidity, &pressure, &windDirection, &windSpeed, &gustSpeed,
                          &pm10_standard,       // Standard PM1.0
                          &pm25_standard,       // Standard PM2.5
                          &pm100_standard,      // Standard PM10.0
                          &pm10_env,            // Environmental PM1.0
                          &pm25_env,            // Environmental PM2.5
                          &pm100_env,           // Environmental PM10.0
                          &particles_03um,      // 0.3um Particle Count
                          &particles_05um,      // 0.5um Particle Count
                          &particles_10um,      // 1.0um Particle Count
                          &particles_25um,      // 2.5um Particle Count
                          &particles_50um,      // 5.0um Particle Count
                          &particles_100um,     // 10.0um Particle Count
                          &pm25aqi              // 24 hours average of PM2.5 data
                      );
  if( scanned != 22 )
  {
    Serial.print( "error parsing input, num: " );
    Serial.println( scanned );
  }
  else
  {
//    char buf[512];
//    sprintf( buf, "%02d:%02d:%02d, %g, %d, %g, %g, %g, %g\n", tm_hour, tm_min, tm_sec, tempF, humidity, pressure, windDirection, windSpeed, gustSpeed );
//    Serial.println( buf );
    rtc_aqi_pm24 = pm25aqi;
  }
  
  display.begin();
  display.clearBuffer();
  display.fillScreen( EPD_WHITE );

  display.drawBitmap( 0, -1, s_folabs_logo, 128, 32, EPD_BLACK );

  u8g2Fonts.setFont( u8g2_font_helvB10_tf );
  drawString( 40, 12, "Far Out Labs", LEFT );

  u8g2Fonts.setForegroundColor( EPD_RED );
  u8g2Fonts.setFont( u8g2_font_helvR08_tf );
  drawString( 40 + 120, 5, "K6LOT-13", LEFT );

  char update_time[32];
  strftime( update_time, sizeof( update_time ), "%I:%M %p", &timeinfo );

  // see if the time is leading with a 0 and replace with a space to keep spacing
  if( update_time[0] == '0' )
    update_time[0] = ' ' ;
  
  drawTimeTempAndHumidity( 4, 32, update_time, tempF, humidity );
  
  // graph
  int windTop = 43 + kPadding;
  int windBottom = display.height() - 6;

  int csize = windBottom - windTop;
  int middle = csize / 2;
  int xCenter = 2 + middle;
  int yCenter = windTop + middle;
  drawWindIndicator( 8, windTop, windBottom - windTop, windDirection, windSpeed, gustSpeed );

  addPressurePoint( pressure );
  drawPressure( 9 + tempOffsetX, 53, pressure );
  drawPressureGraph( 6 + tempOffsetX, 67, kMaxGraphPoints, kGraphHeight ); 

  displayPM25aqi( 6 + tempOffsetX, 72 + kGraphHeight + 2 );

  drawRSSI( 10 + tempOffsetX + kMaxGraphPoints + kPadding, 67, s_wifi_signal );
  drawBattery( 14 + 40 + 120, 28 );

#ifdef USE_SD_CARD
  // my thought is that I could always use data on the SD card to configure this display... for now disabled test code...
  File inputFile;
  inputFile = SD.open("/welcome.txt");
  if ( inputFile == (int)NULL )
  {
    Serial.print( F("File not found") );
    return;
  }

  // status line, with test code to read from file...
  char sdbuffer[1024] = {0};
  int bytesRead = inputFile.read( sdbuffer, sizeof( sdbuffer ) );
  sdbuffer[bytesRead] = 0;
  inputFile.close();
  display.setCursor( 4, 17 );
  display.println( sdbuffer );
#endif

  // send to e-paper display
  display.display();
}


void drawWindIndicator( int x, int y, int height, float windDirection, float windSpeed, float windGust )
{
  int graphTop    = y;
  int graphBottom = y + height;

  // draw wind indicator
  int csize = graphBottom - graphTop;
  int r = csize / 2;
  int middle = csize / 2;
  int ringWidth = 5;
  int rInner = r - (ringWidth - 1);
  int indicatorWidth = 12; // in degrees
  int xCenter = x + 2 + middle;
  int yCenter = graphTop + middle;
  float oversampling = 8.0f;

  for( int i = 0; i < indicatorWidth * oversampling; i++ )
  {
    // convert from math to compass orientation
    int degrees = 90 + (360 - windDirection);
    
    // offset the marker so that it is centered about its target
    int offset = (i + (degrees * oversampling)) - ((indicatorWidth * oversampling) / 2);

    // convert from degrees to radians
    float rs = r * sin( (offset / oversampling) * M_PI / 180.0f );
    float rc = r * cos( (offset / oversampling) * M_PI / 180.0f );
    
    float irs = rInner * sin( (offset / oversampling) * M_PI / 180.0f );
    float irc = rInner * cos( (offset / oversampling) * M_PI / 180.0f );

    float xStart = xCenter + irc;
    float yStart = yCenter - irs;

    float xEnd = xCenter + rc;
    float yEnd = yCenter - rs;

    display.drawLine( xStart, yStart, xEnd, yEnd, EPD_RED );
  }

  display.drawCircle( xCenter, yCenter, r, EPD_BLACK );
  display.drawCircle( xCenter, yCenter, r + 1, EPD_BLACK );
  display.drawCircle( xCenter, yCenter, r - ringWidth, EPD_BLACK );

  int offset = 0;
  char smallbuf[32];

  if( windSpeed > 15 )
    u8g2Fonts.setForegroundColor( EPD_RED );
  else  
    u8g2Fonts.setForegroundColor( EPD_BLACK );
  u8g2Fonts.setFont( u8g2_font_helvB10_tf );
  sprintf( smallbuf, "%.0f", windSpeed );
  if( strlen( smallbuf ) >= 2 )
    offset = 2;
  drawString( xCenter - offset, yCenter - 10, smallbuf, CENTER );

  if( windGust > 15 )
    u8g2Fonts.setForegroundColor( EPD_RED );
  else  
    u8g2Fonts.setForegroundColor( EPD_BLACK );
  u8g2Fonts.setFont( u8g2_font_helvR10_tf );
  sprintf( smallbuf, "%.0f", windGust );
  if( strlen( smallbuf ) >= 2 )
    offset = 2;
  drawString( xCenter - offset, yCenter + 5, smallbuf, CENTER ); 
}



void drawTimeTempAndHumidity( int x, int y, const char* timeString, float tempF, uint8_t humidity )
{
  // there are three zones on one line
  char smallbuf[32];
  u8g2Fonts.setForegroundColor( EPD_BLACK );
  u8g2Fonts.setFont( u8g2_font_helvR08_tf );
  drawString( x + 11, y, timeString, LEFT );

  if( tempF >= 100 )
      u8g2Fonts.setForegroundColor( EPD_RED );
  else
    u8g2Fonts.setForegroundColor( EPD_BLACK );

  u8g2Fonts.setFont( u8g2_font_helvB12_tf );
  sprintf( smallbuf, "%.0f°F", tempF );
  drawString( x + tempOffsetX, y, smallbuf, LEFT );

  draw_single_raindrop( x + 155, y + 4, 10 );


  if( humidity == 100 )
    u8g2Fonts.setForegroundColor( EPD_RED );
  else
    u8g2Fonts.setForegroundColor( EPD_BLACK );

  sprintf( smallbuf, "%3d%%", humidity );
  drawString( x + 165, y, smallbuf, LEFT );   
}



//#########################################################################################
void drawRSSI( int x, int y, int rssi ) 
{
  int WIFIsignal = 0;
  int xpos = 1;
  int x_offset = 0;

//  Serial.println( "rssi = " + String( rssi ) );
  if( rssi > -20 )
    x_offset = -3; // better center the graph when we have 5 bars
    
  for( int _rssi = -100; _rssi <= rssi; _rssi = _rssi + 20 ) 
  {
    if (_rssi <= -20)  WIFIsignal = 20; //            <-20dbm displays 5-bars
    if (_rssi <= -40)  WIFIsignal = 16; //  -40dbm to  -21dbm displays 4-bars
    if (_rssi <= -60)  WIFIsignal = 12; //  -60dbm to  -41dbm displays 3-bars
    if (_rssi <= -80)  WIFIsignal = 8;  //  -80dbm to  -61dbm displays 2-bars
    if (_rssi <= -100) WIFIsignal = 4;  // -100dbm to  -81dbm displays 1-bar
    display.fillRect( x + x_offset + xpos * 6, y - WIFIsignal, 5, WIFIsignal, EPD_BLACK );
    xpos++;
  }
  display.fillRect( x + x_offset, y - 1, 5, 1, EPD_BLACK );
  u8g2Fonts.setFont( u8g2_font_tom_thumb_4x6_tf );
  drawString( x + 17,  y, String( rssi ) + "dBm", CENTER );
}


//#########################################################################################
void drawBattery( int x, int y )
{
  uint8_t percentage = 100;
  float voltage = analogRead( VREF_PIN ) / 4096.0 * 7.46;

  if( voltage > 1 ) 
  { 
    // Only display if there is a valid reading
    int16_t color = EPD_BLACK;
    
    percentage = 2836.9625 * pow( voltage, 4 ) - 43987.4889 * pow( voltage, 3 ) + 255233.8134 * pow( voltage, 2 ) - 656689.7123 * voltage + 632041.7303;
    if( voltage >= 4.20 ) 
      percentage = 100;
      
    if( voltage <= 3.50 ) 
      percentage = 0;

    Serial.println( "Voltage = " + String( voltage ) + " " + String( percentage ) + "%" );

    // do not draw the battery if it is more than 1/2 full
    if( percentage > BATTERY_VID_THRESHOLD )
      return;

    if( percentage <= LOW_BATTERY_THRESHOLD )
      color = EPD_RED;

    display.drawRect( x + 15, y - 12, 19, 9, EPD_BLACK );
    display.fillRect( x + 13, y - 10, 2, 5, EPD_BLACK );
    display.fillRect( x + 17, y - 10, 15 * percentage / 100.0, 5, color );
//    u8g2Fonts.setFont( u8g2_font_tom_thumb_4x6_tf );
//    drawString( x + 10, y - 11, String( percentage ) + "%", RIGHT );
//    drawString( x + 13, y + 5,  String( voltage, 2 ) + "v", CENTER );
  }
}


//#########################################################################################
void drawBatteryV( int x, int y )
{
  uint8_t percentage = 100;
//  float voltage = analogRead( VREF_PIN ) / 4096.0 * 7.46;
  float voltage = 3.9;
  if( voltage > 1 ) 
  { 
    // Only display if there is a valid reading
    Serial.println( "Voltage = " + String( voltage ) );

    int16_t color = EPD_BLACK;
    
    percentage = 2836.9625 * pow( voltage, 4 ) - 43987.4889 * pow( voltage, 3 ) + 255233.8134 * pow( voltage, 2 ) - 656689.7123 * voltage + 632041.7303;
    if( voltage >= 4.20 ) 
      percentage = 100;
      
    if( voltage <= 3.50 ) 
      percentage = 0;

    if( percentage < LOW_BATTERY_THRESHOLD )
      color = EPD_RED;
      
    display.drawRect( x - 12, y + 15, 10, 19, EPD_BLACK );
    display.fillRect( x - 10, y + 34, 5,   2, EPD_BLACK );
    display.fillRect( x - 10, y + 17, 6, 15 * percentage / 100.0, color );
    u8g2Fonts.setFont( u8g2_font_tom_thumb_4x6_tf );
    drawString( x - 11, y + 10, String( percentage ) + "%", RIGHT );
    drawString( x + 5,  y + 13, String( voltage, 2 ) + "v", CENTER );
  }
}


void drawDottedLineV( int16_t x, int16_t y, int16_t h )
{
  int number_of_dashes = h / 2; // calculate this based on the line length divided by 2

  // Draw dotted grid lines
  for( int j = 0; j < number_of_dashes; j++ ) 
  {     
      display.drawFastVLine( x, y + (j * 2), 1, GxEPD_BLACK );
  }
}


// !!@ would be cool to set pressure to red when we detect fast falling or rising
void drawPressure( int x, int y, float baroInHg )
{
  u8g2Fonts.setForegroundColor( EPD_BLACK );
  
  // there are three zones on one line
  char smallbuf[32];
  u8g2Fonts.setFont( u8g2_font_helvB12_tf );

  draw_pressure_icon( x, y - 4, 10 );

  sprintf( smallbuf, "%0.2f", baroInHg );
  drawString( x + 10, y, smallbuf, LEFT );   

  // pressure is fixed field xx.yy so we don't need to worry about proportional spacing
  u8g2Fonts.setFont( u8g2_font_helvR08_tf );
  drawString( x + 10 + 45, y, "inHg", LEFT );   
}


void drawPressureGraph( int x, int y, int width, int height )
{
  int graphTop    = y;
  int graphBottom = y + height;
  int graphWidth  = width - 1;
  int lastH       = graphBottom - 1;

  float minPressure = 100;
  float maxPressure = 0;

  // find min/max to normalize data to 0..1
  for ( int i = 0; i < rtc_graph_count; i++ )
  {
    if( rtc_graph[i] > maxPressure )
      maxPressure = rtc_graph[i];
      
    if( rtc_graph[i] < minPressure )
      minPressure = rtc_graph[i];
  }  

  // draw some divider lines to split the display into thirds (i.e. roughly 2 hour chunks)
  float divider = width / 3;
  drawDottedLineV( x + divider, y, height );
  drawDottedLineV( x + divider * 2, y, height );

  float range = maxPressure - minPressure;
  if( range == 0.0f )
    range = 0.15;  // prevent divide by zero - note this value is chosen to almost center a random pressure value...
    
  for ( int i = 0; i < rtc_graph_count; i++ )
  {
    float r = (rtc_graph[i] - minPressure) / range;
    int   h = (graphBottom - 1) - (r * (height - 1));
    if( i == 0 )
      lastH = h;  // start at the first point instead of at zero

    // draw graph backwards (newest data last)
    display.drawLine( x + graphWidth - i, lastH, x + graphWidth - i, h, EPD_BLACK );
    lastH = h;
  }

  // frame the graph
  display.drawRect( x, y, width, height, EPD_BLACK );

  // cheat a bit and draw outside our frame
  char smallbuf[32];
  sprintf( smallbuf, "%0.2f", maxPressure );
  u8g2Fonts.setFont( u8g2_font_p01type_tn );
  drawString( x - 19, y - 3, smallbuf, LEFT );

  sprintf( smallbuf, "%0.2f", minPressure );
  u8g2Fonts.setFont( u8g2_font_p01type_tn );
  drawString( x - 19, y + height - 11, smallbuf, LEFT );

  float graphMinutes = rtc_graph_count * 5;
  sprintf( smallbuf, "%.1f hrs", graphMinutes / 60.0f );
  u8g2Fonts.setFont( u8g2_font_tom_thumb_4x6_tf );
  drawString( x + width + 4, y + height - 9, smallbuf, LEFT );
}


void addPressurePoint( float pressure )
{
    // see if there's room to move stuff without truncating
    if( rtc_graph_count )
    {
        if( rtc_graph_count < kMaxGraphPoints - 1 )
        {
            // copy only what we have, which will be faster than copying the entire buffer every add
            memmove( &rtc_graph[1], &rtc_graph[0], rtc_graph_count * sizeof( float ) );
        }
        else
        {
            // the buffer is full so we need to move a shortened version of the buffer (which effectively truncates it)
            memmove( &rtc_graph[1], &rtc_graph[0], (rtc_graph_count - 1) * sizeof( float ) );
        }
    }
    
    // stop buffering if we have a big enough window, this can grow but never shrink
    if( rtc_graph_count < (kMaxGraphPoints - 1) )
        ++rtc_graph_count;

    // put the actual data in there now
    memcpy( rtc_graph, &pressure, sizeof( float ) );
}


//#########################################################################################

const break_element_t* concentrationToAQI( float concentration, const break_element_t breaklist[] )
{
  // look up the concentration
  for( int i = 0; i < kNumBreaklistElements; i++ )
  {
    if( concentration >= breaklist[i].loBreak && concentration <= breaklist[i].hiBreak )
      return &breaklist[i];
  }
  return NULL;
}


void displayPM25aqi( int x, int y )
{
  // first calculate the AQI by looking up this value in our table
  const break_element_t* entry = concentrationToAQI( rtc_aqi_pm24, s_break_list_pm25 );
  if( !entry )
    return;
  
  // now do the actual calculation
  float aqi = (entry->hiAQI - entry->loAQI) / (entry->hiBreak - entry->loBreak) * (rtc_aqi_pm24 - entry->loBreak) + entry->loAQI;
  drawAQI( x, y, (int16_t)(aqi + 0.5f), entry->category, "PM2.5" );
}


void drawAQI( int x, int y, int16_t aqi, const char* category, const char* pm_name )
{
  u8g2Fonts.setForegroundColor( GxEPD_BLACK );
  
  char smallbuf[32];
  u8g2Fonts.setFont( u8g2_font_helvB08_tf );
  sprintf( smallbuf, "%d AQI - %s", aqi, category );
  drawString( x - 19, y, smallbuf, LEFT );   
}


void drawAQIGraph( int x, int y, int width, int height, size_t count, int16_t* dataset, const char* graph_name )
{
  int graphTop    = y;
  int graphBottom = y + height;
  int graphWidth  = width - 1;
  int lastH       = graphBottom - 1;

  int16_t minAQI = 20000;
  int16_t maxAQI = 0;

  // find min/max to normalize data to 0..1
  for ( size_t i = 0; i < count; i++ )
  {
    if( dataset[i] > maxAQI )
      maxAQI = dataset[i];
      
    if( dataset[i] < minAQI )
      minAQI = dataset[i];
  }  

  float range = maxAQI - minAQI;
  if( range == 0.0f )
    range = 0.15;  // prevent divide by zero - note this value is chosen to almost center a random value...

  // stride is 2 because we average the two values together so that we can display 24 hours in a 12 hour space
  for ( size_t i = 0; i < count; i += 2 )
  {
    int16_t measurement;
    if( i + 1 < count )
      measurement = (dataset[i] + dataset[i + 1]) / 2;
    else
      measurement = dataset[i];
  
    float r = (measurement - minAQI) / range;
    int   h = (graphBottom - 1) - (r * (height - 1));

    // draw graph backwards (newest data last)
    display.drawLine( x + graphWidth - i, lastH, x + graphWidth - i, h, GxEPD_BLACK );
    lastH = h;
  }

  // frame the graph
  display.drawRect( x, y, width, height, GxEPD_BLACK );

  // cheat a bit and draw outside our frame
  char smallbuf[32];
  sprintf( smallbuf, "%5d", maxAQI );
  u8g2Fonts.setFont( u8g2_font_p01type_tn );
  drawString( x - 21, y - 3, smallbuf, LEFT );

  sprintf( smallbuf, "%5d", minAQI );
  u8g2Fonts.setFont( u8g2_font_p01type_tn );
  drawString( x - 21, y + height - 11, smallbuf, LEFT );

  u8g2Fonts.setFont( u8g2_font_p01type_tf );
  drawString( x, y - 11, graph_name, LEFT );
}


void addAQIPoint( int16_t pmCount, size_t* count, int16_t* dataset )
{
    if( !count || !dataset )
      return;

    size_t dataset_count = *count;
    
    // see if there's room to move stuff without truncating
    if( dataset_count )
    {
        if( dataset_count < kMaxAQIDataPoints - 1 )
        {
            // copy only what we have, which will be faster than copying the entire buffer every add
            memmove( &dataset[1], &dataset[0], dataset_count * sizeof( pmCount ) );
        }
        else
        {
            // the buffer is full so we need to move a shortened version of the buffer (which effectively truncates it)
            memmove( &dataset[1], &dataset[0], (dataset_count - 1) * sizeof( pmCount ) );
        }
    }
    
    // stop buffering if we have a big enough window, this can grow but never shrink
    if( dataset_count < (kMaxAQIDataPoints - 1) )
        *count = dataset_count + 1; 

    // put the actual data in there now
    memcpy( dataset, &pmCount, sizeof( pmCount ) );
}





void loop()
{
}


// EOF
