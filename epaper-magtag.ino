
///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2021 Far Out Labs.  All rights reserved.
//      This material is the confidential trade secret and proprietary
//      information of FOLabs.  It may not be reproduced, used,
//      sold or transferred to any third party without the prior written
//      consent of FOLabs.  All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

/*  History:

13sept21 alx Cleaned up for the public

*/


// Set Arduino Board to "Adafruit ESP32 MagTag 2.9"" - all defaults...

#include <SPI.h>
#include <Wire.h>
#include <time.h>

#include "Adafruit_ThinkInk.h"
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <U8g2_for_Adafruit_GFX.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>

#include "wifi_password.h"
#include "icons.h"


// define this to have the device go to sleep instead of going idle.  Wake with the A button.
// note: not really well tested-  use at your own risk...
//#define SLEEP_ENABLED


#define EPD_CS           8
#define EPD_DC           7
#define SRAM_CS         -1
#define EPD_RESET        6   // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY         5   // can set to -1 to not use a pin (will wait a fixed delay)
#define SD_CS           -1
#define VREF_PIN         4
#define NEOPIX_POWER_PIN GPIO_NUM_21
#define NEOPIX_PIN       GPIO_NUM_1

#define BATTERY_VID_THRESHOLD 50   // percent
#define LOW_BATTERY_THRESHOLD 10   // percent

#define PIXEL_COUNT       4        // Number of NeoPixels on MagTag
#define PIXEL_BRIGHTNESS 16

#define BUTTON_A GPIO_NUM_15
#define BUTTON_B GPIO_NUM_14
#define BUTTON_C GPIO_NUM_12
#define BUTTON_D GPIO_NUM_11

#define kFoodIconWidth        74
#define kFoodIconHeight       74

#define kTimeoutThresholdSecs 8


typedef struct
{
    const uint8_t PIN;
    bool          pressed;
} ButtonRecord;

typedef struct
{
  uint8_t hour; 
  uint8_t min;
  uint8_t sec;
  uint8_t day;
} TimeRecord;


enum button_state 
{ 
  kFoodSnackBit     = 1 << 0, 
  kFoodBreakfastBit = 1 << 1, 
  kFoodLunchBit     = 1 << 2, 
  kFoodDinnerBit    = 1 << 3 
};

enum alignment
{
  LEFT, 
  RIGHT, 
  CENTER
};


static const char* s_ssid     = STASSID;
static const char* s_password = STAPSK;

static const char* s_timezone            = "PST8PDT,M3.2.0,M11.1.0";
static const char* s_ntp_server          = "pool.ntp.org";
static int         s_gmt_offset_sec      = -25200; // GMT Offset is 0, for US (-5Hrs) is typically -18000, AU is typically (+8hrs) 28800
static int         s_daylight_offset_sec = 3600;   // In the US/UK DST is +1hr or 3600-secs, other countries may use 2hrs 7200 or 30-mins 1800 or 5.5hrs 19800 Ahead of GMT use + offset behind - offset

static long s_sleep_duration_mins = 60;  // Sleep time in minutes (1 hour so that we can reset the states promptly at midnight)
static int  s_wifi_signal = 0;

static String     s_date_str;
static uint8_t    s_startup_day    = 0;
static TimeRecord s_last_update    = {};
static TimeRecord s_last_snack     = {};
static TimeRecord s_last_breakfast = {};
static TimeRecord s_last_lunch     = {};
static TimeRecord s_last_dinner    = {};

static const char* kFoodSnack     = "Snack";
static const char* kFoodBreakfast = "Breakfast";
static const char* kFoodLunch     = "Lunch";
static const char* kFoodDinner    = "Dinner";

static bool s_idle = false;
static bool s_display_needs_refresh = false;

RTC_DATA_ATTR uint8_t rtc_button_state = 0;
RTC_DATA_ATTR uint8_t rtc_snack_count  = 0;

RTC_DATA_ATTR ButtonRecord button1 = { BUTTON_A, false };
RTC_DATA_ATTR ButtonRecord button2 = { BUTTON_B, false };
RTC_DATA_ATTR ButtonRecord button3 = { BUTTON_C, false };
RTC_DATA_ATTR ButtonRecord button4 = { BUTTON_D, false };

#ifdef SLEEP_ENABLED
RTC_DATA_ATTR bool rtc_booted = false;
#endif

// static classes
Adafruit_NeoPixel strip( PIXEL_COUNT, NEOPIX_PIN, NEO_GRB + NEO_KHZ800 );

// 2.9" Grayscale Featherwing or Breakout:
ThinkInk_290_Grayscale4_T5 display( EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY );

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;  // Select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall


//#########################################################################################
// code

void drawString( int x, int y, String text, alignment alignment ) 
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


void drawTime( int x, int y, TimeRecord* tm )  
{
  int16_t  x1, y1; //the bounds of x,y and w and h of the variable 'text' in pixels.
  uint16_t w, h;

  char update_time[30];
  struct tm timeInfo = {0};
  timeInfo.tm_hour = tm->hour;
  timeInfo.tm_min  = tm->min;
  timeInfo.tm_sec  = tm->sec;
  timeInfo.tm_mday = tm->day;
  strftime( update_time, sizeof( update_time ), "%I:%M%p", &timeInfo );
  
  display.setTextWrap( false );
  display.getTextBounds( update_time, x, y, &x1, &y1, &w, &h );
  u8g2Fonts.setCursor( x, y + h );
  u8g2Fonts.print( update_time );
}


uint16_t getTextWidth( String text ) 
{
  int16_t  x1, y1; 
  uint16_t w, h;
  display.setTextWrap(false);
  display.getTextBounds( text, 0, 0, &x1, &y1, &w, &h );
  return w;
}


boolean setupTime() 
{
  configTime( s_gmt_offset_sec, s_daylight_offset_sec, s_ntp_server, "time.nist.gov");
  setenv("TZ", s_timezone, 1);  //setenv()adds the "TZ" variable to the environment with a value TimeZone, only used if set to 1, 0 means no change
  tzset(); // Set the TZ environment variable
  delay(100);
  bool TimeStatus = updateLocalTime();
  s_startup_day = s_last_update.day;
  return TimeStatus;
}


boolean updateLocalTime() 
{
  struct tm timeinfo;
  char   time_output[30], day_output[30], update_time[30];
  while( !getLocalTime( &timeinfo, 5000 ) ) 
  { 
    // Wait for 5-sec for time to synchronise
    Serial.println( "Failed to obtain time" );
    return false;
  }

  s_last_update.hour = timeinfo.tm_hour;
  s_last_update.min  = timeinfo.tm_min;
  s_last_update.sec  = timeinfo.tm_sec;
  s_last_update.day  = timeinfo.tm_mday;
  Serial.println( &timeinfo, "%a %b %d %Y   %H:%M:%S" );      // Displays: Saturday, June 24 2017 14:05:49

// see http://www.cplusplus.com/reference/ctime/strftime/
  strftime(update_time, sizeof(update_time), "%A %B %e %Y", &timeinfo);    // Creates: 'Monday September  6 2021'
  s_date_str = update_time;
  return true;
}


bool checkForTimeout()
{
  struct tm timeinfo;
  while( !getLocalTime( &timeinfo, 5000 ) ) 
  { 
    // Wait for 5-sec for time to synchronise
    Serial.println( "Failed to obtain time" );
    return false;
  }

  // check to see if we moved onto a new day...
  if( s_startup_day && (timeinfo.tm_mday != s_startup_day) )
  {
    // reset all the icons...and date
    Serial.println( "Processing date change: reseting all icons..." );
    rtc_button_state = 0;
    rtc_snack_count = 0;
    s_startup_day = timeinfo.tm_mday;
    s_display_needs_refresh = true;
  }

  // convert everything to seconds...
  uint32_t thenSecs = (s_last_update.hour * 3600) + (s_last_update.min * 60) + s_last_update.sec;
  uint32_t nowSecs  = (timeinfo.tm_hour * 3600) + (timeinfo.tm_min * 60) + timeinfo.tm_sec;

  // do a simple compare 
  return (nowSecs - thenSecs) > kTimeoutThresholdSecs;
}


void goIdle()
{
  if( s_idle )
    return;
    
  // turn off the led if on
  digitalWrite( LED_BUILTIN, HIGH );

  strip.clear();
  strip.show();

  Serial.println( "Going idle..." );
  s_idle = true;
}


void beginSleep()
{
  goIdle();

  pinMode( LED_BUILTIN, INPUT );

  // turn off power to neopixels
  digitalWrite( NEOPIX_POWER_PIN, LOW );
  pinMode( NEOPIX_POWER_PIN, INPUT );

  Serial.println( "Starting deep-sleep period..." );
  esp_sleep_enable_ext0_wakeup( BUTTON_A, 0 ); // 1 = High, 0 = Low
   
  // put microcontroller to sleep, wake up after specified time - just to update the date
  // button presses will also wake processor
  ESP.deepSleep( s_sleep_duration_mins * 60 * 1e6 );
}


void ARDUINO_ISR_ATTR isr( void* arg ) 
{
    ButtonRecord* s = static_cast<ButtonRecord*>(arg);
    s->pressed = true;
}


void setNeoPixelsColor( uint32_t color ) 
{
  for( int i=0; i < strip.numPixels(); i++ ) 
    strip.setPixelColor(i, color);          //  Set pixel's color (in RAM)
  
  strip.show();
}


void updateIconState( uint8_t foodBit, bool foodGiven )
{
  if( foodGiven )
    rtc_button_state |= foodBit;
  else
    rtc_button_state &= ~foodBit;

  // now redraw the display...
  s_display_needs_refresh = true;
}


bool getIconState( uint8_t foodBit )
{
  return rtc_button_state & foodBit;
}


//#########################################################################################

void setup(void)
{
  Serial.begin( 115200 );
//  while (!Serial);   // time to get serial running

  pinMode( LED_BUILTIN, OUTPUT );

  pinMode( BUTTON_A, INPUT_PULLUP );
  pinMode( BUTTON_B, INPUT_PULLUP );
  pinMode( BUTTON_C, INPUT_PULLUP );
  pinMode( BUTTON_D, INPUT_PULLUP );
  attachInterruptArg( button1.PIN, isr, &button1, FALLING );
  attachInterruptArg( button2.PIN, isr, &button2, FALLING );
  attachInterruptArg( button3.PIN, isr, &button3, FALLING );
  attachInterruptArg( button4.PIN, isr, &button4, FALLING );

  // turn on power to the NeoPixels.
  pinMode( NEOPIX_POWER_PIN, OUTPUT );
  digitalWrite( NEOPIX_POWER_PIN, LOW );

  strip.begin(); // Initialize NeoPixel strip object (REQUIRED)
  strip.setBrightness( PIXEL_BRIGHTNESS );
  strip.show();  // Initialize all pixels to 'off'
  
  u8g2Fonts.begin(display);                  // connect u8g2 procedures to Adafruit GFX
  u8g2Fonts.setFontMode(1);                  // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(0);             // left to right (this is default)
  u8g2Fonts.setForegroundColor(EPD_BLACK); // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(EPD_WHITE); // apply Adafruit GFX color

#ifdef SLEEP_ENABLED
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  // only turn on lights if user woke us up...
  if( wakeup_reason != ESP_SLEEP_WAKEUP_TIMER )
    setNeoPixelsColor( strip.Color(127, 127, 127) );

  // draw first (without the date) so the device responds sooner
  draw_epd( false );
#endif
  
  startWiFi(); // this routine keeps on trying in poor wifi conditions...
  setupTime();

#ifdef SLEEP_ENABLED
  // draw only once at boot time, the next time we start up, we don't need to update the screen
  // unless a button is pressed...
  if( !rtc_booted || (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) )
  {
    draw_epd( true );
    rtc_booted = true;
  }
#else
  draw_epd( true );
#endif
}


void loop()
{
  // if there is a button press, reset the time...so we don't time out
  // button presses come from interrupts...
  if( button1.pressed || button2.pressed || button3.pressed || button4.pressed )
  {
#ifndef SLEEP_ENABLED
    setNeoPixelsColor( strip.Color(127, 127, 127) );
    s_idle = false;
#endif    
    updateLocalTime();

    // toggle the states
    if( button1.pressed )
    {
      s_last_snack = s_last_update;
      ++rtc_snack_count;
      updateIconState( kFoodSnackBit, rtc_snack_count );
      button1.pressed = false;
    }      
    else if( button2.pressed )
    {
      s_last_breakfast = s_last_update;
      updateIconState( kFoodBreakfastBit, !getIconState( kFoodBreakfastBit ) );
      button2.pressed = false;
    }
    else if( button3.pressed )
    {
      s_last_lunch = s_last_update;
      updateIconState( kFoodLunchBit, !getIconState( kFoodLunchBit ) );
      button3.pressed = false;
    }
    else if( button4.pressed )
    {
      s_last_dinner = s_last_update;
      updateIconState( kFoodDinnerBit, !getIconState( kFoodDinnerBit ) );
      button4.pressed = false;
    }
  }

  if( s_display_needs_refresh )
  {
    draw_epd( true );
    s_display_needs_refresh = false;
  }

  // after no activity, go idle-- !!@ in the future if we can detect we are on a battery, we would sleep here...
  if( checkForTimeout() )
  {
#ifdef SLEEP_ENABLED
    beginSleep();
#else
    goIdle();
#endif
  }
}


void draw_epd( bool draw_date )
{
  Serial.println("EPD begun");

  display.begin();
  display.clearBuffer();
  display.fillScreen( EPD_WHITE );

  display.drawBitmap( 0, -1, s_folabs_logo, 128, 32, EPD_BLACK );

  u8g2Fonts.setFont( u8g2_font_helvB10_tf );
  drawString( 40, 12, "Far Out Labs  --  Dog Food Tracker", LEFT );

  u8g2Fonts.setForegroundColor( EPD_BLACK );
  u8g2Fonts.setFont( u8g2_font_helvR08_tf );

  uint16_t w = getTextWidth( "Snack" );
  uint16_t centered = (kFoodIconWidth - w) / 2;
  const uint8_t* bitmap = s_Snack;
  uint16_t       grey   = rtc_button_state & kFoodSnackBit ? EPD_GRAY : EPD_BLACK;
  display.drawBitmap( 0, 40, bitmap, kFoodIconWidth, kFoodIconHeight, grey );
  drawString( centered, 40 + 75, String( "Snack" ), LEFT );
  if( rtc_snack_count )
  {
      drawTime( centered - 6, 40 + 65, &s_last_snack );
      u8g2Fonts.setForegroundColor( EPD_GRAY );
      u8g2Fonts.setFont( u8g2_font_helvB10_tf );
      drawString( centered + 25, 56, String( rtc_snack_count ), LEFT );
  }

  u8g2Fonts.setForegroundColor( EPD_BLACK );
  u8g2Fonts.setFont( u8g2_font_helvR08_tf );
  w = getTextWidth( "Breakfast" );
  centered = (kFoodIconWidth - w) / 2;  
  bitmap = rtc_button_state & kFoodBreakfastBit ? s_BowlEmpty : s_BowlFull;
  grey   = rtc_button_state & kFoodBreakfastBit ? EPD_GRAY : EPD_BLACK;
  display.drawBitmap( kFoodIconWidth, 40, bitmap, kFoodIconWidth, kFoodIconHeight, grey );
  drawString( kFoodIconWidth + centered + 6, 40 + 75, String( "Breakfast" ), LEFT );
  if( rtc_button_state & kFoodBreakfastBit )
      drawTime( kFoodIconWidth + centered + 7, 40 + 65, &s_last_breakfast );
  
  w = getTextWidth( "Lunch" );
  centered = (kFoodIconWidth - w) / 2;  
  bitmap = rtc_button_state & kFoodLunchBit ? s_BowlEmpty : s_BowlFull;
  grey   = rtc_button_state & kFoodLunchBit ? EPD_GRAY : EPD_BLACK;
  display.drawBitmap( kFoodIconWidth * 2, 40, bitmap, kFoodIconWidth, kFoodIconHeight, grey );
  drawString( kFoodIconWidth * 2 + centered, 40 + 75, String( "Lunch" ), LEFT );
  if( rtc_button_state & kFoodLunchBit )
      drawTime( kFoodIconWidth * 2 + centered - 6, 40 + 65, &s_last_lunch );

  w = getTextWidth( "Dinner" );
  centered = (kFoodIconWidth - w) / 2;  
  bitmap = rtc_button_state & kFoodDinnerBit ? s_BowlEmpty : s_BowlFull;
  grey   = rtc_button_state & kFoodDinnerBit ? EPD_GRAY : EPD_BLACK;
  display.drawBitmap( kFoodIconWidth * 3, 40, bitmap, kFoodIconWidth, kFoodIconHeight, grey );
  drawString( kFoodIconWidth * 3 + centered + 3, 40 + 75, String( "Dinner" ), LEFT );
  if( rtc_button_state & kFoodDinnerBit )
      drawTime( kFoodIconWidth * 3 + centered - 3, 40 + 65, &s_last_dinner );

  // draw the date
  if( draw_date )
  {
    uint16_t w = getTextWidth( s_date_str );
    uint16_t centered = (display.width() - w) / 2;
    drawString( centered, 30, s_date_str, LEFT );
  }

//  drawRSSI( 230, 14, s_wifi_signal );
//  drawBattery( 255, 10 );

  // send to e-paper display
  display.display();
}




//#########################################################################################

uint8_t startWiFi() 
{
    Serial.print("\nConnecting to: "); Serial.println( String( s_ssid ) );
    WiFi.disconnect();
    WiFi.mode( WIFI_STA );
    WiFi.setAutoConnect( true );
    WiFi.setAutoReconnect( true );
    WiFi.begin( s_ssid, s_password );
    
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


void stopWiFi() 
{
  WiFi.disconnect();
  WiFi.mode( WIFI_OFF );
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
//    if( percentage > BATTERY_VID_THRESHOLD )
//      return;
//
//    if( percentage <= LOW_BATTERY_THRESHOLD )
//      color = EPD_RED;

    display.drawRect( x + 15, y - 2, 19, 9, EPD_BLACK );
    display.fillRect( x + 13, y,      2, 5, EPD_BLACK );
    display.fillRect( x + 17, y, 15 * percentage / 100.0, 5, color );
//    u8g2Fonts.setFont( u8g2_font_tom_thumb_4x6_tf );
//    drawString( x + 10, y - 11, String( percentage ) + "%", RIGHT );
//    drawString( x + 13, y + 5,  String( voltage, 2 ) + "v", CENTER );
  }
}


// EOF
