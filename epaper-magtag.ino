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
#include <time.h>

#include "Adafruit_ThinkInk.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_NeoPixel.h>
#include <U8g2_for_Adafruit_GFX.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>

#include "wifi_password.h"
#include "icons.h"


// define this to have the device go to sleep instead of going idle.  Wake with the A button.
//#define SLEEP_ENABLED


#define EPD_CS     8
#define EPD_DC     7
#define SRAM_CS    -1
#define EPD_RESET  6   // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY   5   // can set to -1 to not use a pin (will wait a fixed delay)
#define SD_CS      -1
#define CONNECTED
#define VREF_PIN   4
#define NEOPIX_POWER_PIN GPIO_NUM_21
#define NEOPIX_PIN       GPIO_NUM_1

#define kPadding   3
#define BATTERY_VID_THRESHOLD 50   // percent
#define LOW_BATTERY_THRESHOLD 10   // percent

#define PIXEL_COUNT  4     // Number of NeoPixels
#define PIXEL_BRIGHTNESS 32

#define BUTTON_A GPIO_NUM_15
#define BUTTON_B GPIO_NUM_14
#define BUTTON_C GPIO_NUM_12
#define BUTTON_D GPIO_NUM_11

// hack
#ifndef GxEPD_BLACK
#define GxEPD_BLACK EPD_BLACK
#endif 

#define kFoodIconWidth  74
#define kFoodIconHeight 74


enum alignmentType {LEFT, RIGHT, CENTER};

static const char*    ssid     = STASSID;
static const char*    password = STAPSK;

static const char* Timezone           = "PST8PDT,M3.2.0,M11.1.0";
static const char* ntpServer          = "pool.ntp.org";
static int         gmtOffset_sec      = -25200; // GMT Offset is 0, for US (-5Hrs) is typically -18000, AU is typically (+8hrs) 28800
static int         daylightOffset_sec = 3600;   // In the US/UK DST is +1hr or 3600-secs, other countries may use 2hrs 7200 or 30-mins 1800 or 5.5hrs 19800 Ahead of GMT use + offset behind - offset

static long s_sleep_duration_mins = 60;  // Sleep time in minutes (1 hour so that we can reset the states promptly at midnight)
static int  s_wifi_signal = 0;

static String  s_date_str;
static int     s_last_update_Hour = 0, s_last_update_Min = 0, s_last_update_Sec = 0, s_last_update_Day = 0;
static long    StartTime = 0;

static int     tempOffsetX = 85;
static int     kTimeoutThresholdSecs = 8; // seconds

enum button_state 
{ 
  kFoodSnackBit     = 1 << 0, 
  kFoodBreakfastBit = 1 << 1, 
  kFoodLunchBit     = 1 << 2, 
  kFoodDinnerBit    = 1 << 3 
};
RTC_DATA_ATTR uint8_t rtc_button_state = 0;

struct Button {
    const uint8_t PIN;
    bool          pressed;
};

RTC_DATA_ATTR Button button1 = { BUTTON_A, false };
RTC_DATA_ATTR Button button2 = { BUTTON_B, false };
RTC_DATA_ATTR Button button3 = { BUTTON_C, false };
RTC_DATA_ATTR Button button4 = { BUTTON_D, false };

static const char* kFoodSnack     = "Snack";
static const char* kFoodBreakfast = "Breakfast";
static const char* kFoodLunch     = "Lunch";
static const char* kFoodDinner    = "Dinner";

RTC_DATA_ATTR bool rtc_booted = false;

// static classes
Adafruit_NeoPixel strip(PIXEL_COUNT, NEOPIX_PIN, NEO_GRB + NEO_KHZ800);

// 2.9" Grayscale Featherwing or Breakout:
ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;  // Select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall


// code


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

  // check to see if we moved onto a new day...
  if( s_last_update_Day && (s_last_update_Day != timeinfo.tm_mday) )
  {
    // reset all the icons...
    rtc_button_state = 0;
  }
  
  s_last_update_Hour = timeinfo.tm_hour;
  s_last_update_Min  = timeinfo.tm_min;
  s_last_update_Sec  = timeinfo.tm_sec;
  s_last_update_Day  = timeinfo.tm_mday;
  Serial.println( &timeinfo, "%a %b %d %Y   %H:%M:%S" );      // Displays: Saturday, June 24 2017 14:05:49
  
  // see http://www.cplusplus.com/reference/ctime/strftime/
  strftime(update_time, sizeof(update_time), "%A %B %e %Y", &timeinfo);    // Creates: 'Monday September  6 2021'
  s_date_str = update_time;
  return true;
}


bool CheckForTimeout()
{
  struct tm timeinfo;
  while( !getLocalTime( &timeinfo, 5000 ) ) 
  { 
    // Wait for 5-sec for time to synchronise
    Serial.println( "Failed to obtain time" );
    return false;
  }

  // do a simple compare
  if( s_last_update_Hour != timeinfo.tm_hour )
    return true;

   if( (s_last_update_Hour == timeinfo.tm_hour) && (timeinfo.tm_min - s_last_update_Min > 1) )
    return true;

   if( (s_last_update_Min == timeinfo.tm_min) && (timeinfo.tm_sec - s_last_update_Sec > kTimeoutThresholdSecs) )
    return true;

  return false;
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


void GoIdle()
{
  // turn off the led if on
  digitalWrite( LED_BUILTIN, HIGH );

  strip.clear();
  strip.show();

  Serial.println( "Going idle..." );
}


void BeginSleep()
{
  GoIdle();

  pinMode( LED_BUILTIN, INPUT );

  // turn off power to neopixels
  digitalWrite( NEOPIX_POWER_PIN, LOW );
  pinMode( NEOPIX_POWER_PIN, INPUT );

  Serial.println( "Starting deep-sleep period..." );
  esp_sleep_enable_ext0_wakeup( BUTTON_A, 0 ); // 1 = High, 0 = Low
   
  // put microcontroller to sleep, wake up after specified time - just to update the date
  // button presses will also wake processor
  ESP.deepSleep( s_sleep_duration_mins * 60 * 1e6 );
//  ESP.deepSleep( 1 * 60 * 1e6 );
}





void ARDUINO_ISR_ATTR isr(void* arg) {
    Button* s = static_cast<Button*>(arg);
    s->pressed = true;
}


void SetNeoPixelsColor(uint32_t color) 
{
  for(int i=0; i<strip.numPixels(); i++) {  // For each pixel in strip...
    strip.setPixelColor(i, color);          //  Set pixel's color (in RAM)
  }
  strip.show();
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(uint32_t color, int wait) {
  for(int a=0; a<10; a++) {  // Repeat 10 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show(); // Update strip with new contents
      delay(wait);  // Pause for a moment
    }
  }
}


void UpdateIconState( uint8_t foodBit, bool foodGiven )
{
  if( foodGiven )
    rtc_button_state |= foodBit;
  else
    rtc_button_state &= ~foodBit;

  // now redraw the display...
  draw_epd( true );
}


bool GetIconState( uint8_t foodBit )
{
  return rtc_button_state & foodBit;
}


void setup(void)
{
  StartTime = millis();
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
    SetNeoPixelsColor( strip.Color(127, 127, 127) );
#endif
  
  // draw first (without the date) so the device responds sooner
//  draw_epd( false );

#ifdef CONNECTED
  StartWiFi(); // this routine keeps on trying in poor wifi conditions...
  SetupTime();
#endif

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
    SetNeoPixelsColor( strip.Color(127, 127, 127) );
#endif    
    UpdateLocalTime();

    // toggle the states
    if( button1.pressed )
      UpdateIconState( kFoodSnackBit, !GetIconState( kFoodSnackBit ) );
    else if( button2.pressed )
      UpdateIconState( kFoodBreakfastBit, !GetIconState( kFoodBreakfastBit ) );
    else if( button3.pressed )
      UpdateIconState( kFoodLunchBit, !GetIconState( kFoodLunchBit ) );
    else if( button4.pressed )
      UpdateIconState( kFoodDinnerBit, !GetIconState( kFoodDinnerBit ) );
  }

  // reset the buttons...
  button1.pressed = false;
  button2.pressed = false;
  button3.pressed = false;
  button4.pressed = false;

  // after no activity, go idle-- !!@ in the future if we can detect we are on a battery, we would sleep here...
  if( CheckForTimeout() )
    GoIdle();
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


void draw_epd( bool draw_date )
{
  Serial.println("EPD begun");

  display.begin();
  display.clearBuffer();
  display.fillScreen( EPD_WHITE );

  display.drawBitmap( 0, -1, s_folabs_logo, 128, 32, EPD_BLACK );

  u8g2Fonts.setFont( u8g2_font_helvB10_tf );
  drawString( 40, 12, "Far Out Labs", LEFT );

//  u8g2Fonts.setForegroundColor( EPD_RED );
//  u8g2Fonts.setFont( u8g2_font_helvR08_tf );
//  drawString( 40 + 120, 10, "K6LOT-13", LEFT );

  u8g2Fonts.setForegroundColor( EPD_BLACK );
  u8g2Fonts.setFont( u8g2_font_helvR08_tf );

  uint16_t w = getTextWidth( "Snack" );
  uint16_t centered = (kFoodIconWidth - w) / 2;
  const uint8_t* bitmap = s_Snack;
  uint16_t       grey   = rtc_button_state & kFoodSnackBit ? EPD_GRAY : EPD_BLACK;
  display.drawBitmap( 0, 40, bitmap, kFoodIconWidth, kFoodIconHeight, grey );
  drawString( centered, 40 + 75, String( "Snack" ), LEFT );

  w = getTextWidth( "Breakfast" );
  centered = (kFoodIconWidth - w) / 2;  
  bitmap = rtc_button_state & kFoodBreakfastBit ? s_BowlEmpty : s_BowlFull;
  grey   = rtc_button_state & kFoodBreakfastBit ? EPD_GRAY : EPD_BLACK;
  display.drawBitmap( kFoodIconWidth, 40, bitmap, kFoodIconWidth, kFoodIconHeight, grey );
  drawString( kFoodIconWidth + centered + 6, 40 + 75, String( "Breakfast" ), LEFT );

  w = getTextWidth( "Lunch" );
  centered = (kFoodIconWidth - w) / 2;  
  bitmap = rtc_button_state & kFoodLunchBit ? s_BowlEmpty : s_BowlFull;
  grey   = rtc_button_state & kFoodLunchBit ? EPD_GRAY : EPD_BLACK;
  display.drawBitmap( kFoodIconWidth * 2, 40, bitmap, kFoodIconWidth, kFoodIconHeight, grey );
  drawString( kFoodIconWidth * 2 + centered, 40 + 75, String( "Lunch" ), LEFT );

  w = getTextWidth( "Dinner" );
  centered = (kFoodIconWidth - w) / 2;  
  bitmap = rtc_button_state & kFoodDinnerBit ? s_BowlEmpty : s_BowlFull;
  grey   = rtc_button_state & kFoodDinnerBit ? EPD_GRAY : EPD_BLACK;
  display.drawBitmap( kFoodIconWidth * 3, 40, bitmap, kFoodIconWidth, kFoodIconHeight, grey );
  drawString( kFoodIconWidth * 3 + centered + 3, 40 + 75, String( "Dinner" ), LEFT );

  // draw the date
  if( draw_date )
  {
    uint16_t w = getTextWidth( s_date_str );
    uint16_t centered = (display.width() - w) / 2;
    drawString( centered, 30, s_date_str, LEFT );
  }

//  drawRSSI( 230, 14, s_wifi_signal );
  drawBattery( 255, 10 );

  // send to e-paper display
  display.display();
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


// EOF
