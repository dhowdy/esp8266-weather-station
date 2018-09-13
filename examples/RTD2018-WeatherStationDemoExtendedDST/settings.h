/**The MIT License (MIT)

Copyright (c) 2016 by Daniel Eichhorn

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

See more at http://blog.squix.ch
*/

/* Modifications made by Don Howdeshell for RTD2018 at Missouri S&T
* github.com/dhowdy dhowdy.blogspot.com
*     
* Added RTD Logo and customization line
* Added Oxygen-Regular size 10 as a splash screen font to coincide with other RTD branding
* Converted the project from using Weather Underground to OpenWeatherMap
* Added the ability to switch between Wundergroundand OWM
* Moved and re-aligned text to work with bicolor LCD
* Fixed a bug with NTP settings
* Americanized the settings
* Added dewpoint status for wunderground (or placeholder for arbitrary data)
* Added wind speed for openweathermap (or placeholder for arbitrary data)
* Data, settings, and other things not needed for the RTD workshop have been removed
* Added option to switch between manual WiFi and WiFi Manager
* TODO: Add option to look up city by OWM City ID
* 
*/

/* Customizations by Neptune (NeptuneEng on Twitter, Neptune2 on Github)
 *  
 *  Added Wifi Splash screen and credit to Squix78
 *  Modified progress bar to a thicker and symmetrical shape
 *  Replaced TimeClient with built-in lwip sntp client (no need for external ntp client library)
 *  Added Daylight Saving Time Auto adjuster with DST rules using simpleDSTadjust library
 *  https://github.com/neptune2/simpleDSTadjust
 *  Added Setting examples for Boston, Zurich and Sydney
  *  Selectable NTP servers for each locale
  *  DST rules and timezone settings customizable for each locale
   *  See https://www.timeanddate.com/time/change/ for DST rules
  *  Added AM/PM or 24-hour option for each locale
 *  Changed to 7-segment Clock font from http://www.keshikan.net/fonts-e.html
 *  Added Forecast screen for days 4-6 (requires 1.1.3 or later version of esp8266_Weather_Station library)
 *  Added support for DHT22, DHT21 and DHT11 Indoor Temperature and Humidity Sensors
 *  Fixed bug preventing display.flipScreenVertically() from working
 *  Slight adjustment to overlay
 */
 
#include <simpleDSTadjust.h>
#include "DHT.h"

// >>> Uncomment one of the following 2 lines to define which OLED display interface type you are using
//#define spiOLED
#define i2cOLED

#ifdef spiOLED
#include "SSD1306Spi.h"
#endif
#ifdef i2cOLED
#include "SSD1306Wire.h"
#endif
#include "OLEDDisplayUi.h"

// Please read http://blog.squix.org/weatherstation-getting-code-adapting-it
// for setup instructions

#define HOSTNAME "ESP8266-weather"

// Setup
const int UPDATE_INTERVAL_SECS = 10 * 60; // Update every 10 minutes

#ifdef spiOLED
// Pin definitions for SPI OLED
#define OLED_CS     D8  // Chip select
#define OLED_DC     D2  // Data/Command
#define OLED_RESET  D0  // RESET  - If you get an error on this line, either change Tools->Board to the board you are using or change "D0" to the appropriate pin number for your board.
#endif

#ifdef i2cOLED
// Pin definitions for I2C OLED
const int I2C_DISPLAY_ADDRESS = 0x3c;
// const int SDA_PIN = 0;
// const int SDC_PIN = 2;
const int SDA_PIN = D3;
const int SDC_PIN = D4;
#endif

// DHT Sensor Settings
// Uncomment whatever type you're using!

// #define DHTPIN D2 // NodeMCU
#define DHTPIN D6 // Wemos D1R2 Mini

#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

#if DHTTYPE == DHT22
#define DHTTEXT "DHT22"
#elif DHTTYPE == DHT21
#define DHTTEXT "DHT21"
#elif DHTTYPE == DHT11
#define DHTTEXT "DHT11"
#endif
char FormattedTemperature[10];
char FormattedHumidity[10];

// Set your name to be displayed on the splash screen
const String AUTHOR_NAME = "Don Howdeshell";

// Manual Wifi
//#define ManualWifi

#ifdef ManualWifi
const char* WIFI_SSID = "yourssid";
const char* WIFI_PASSWORD = "yourpassw0rd";
#endif

//DST rules for US Central Time Zone (St. Louis, Chicago)
#define UTC_OFFSET -6
struct dstRule StartRule = {"CDT", Second, Sun, Mar, 2, 3600}; // Central Daylight time = UTC/GMT -5 hours
struct dstRule EndRule = {"CST", First, Sun, Nov, 1, 0};       // Central Standard time = UTC/GMT -6 hour

// Uncomment for 24 Hour style clock
//#define STYLE_24HR

#define NTP_SERVERS "0.ntp.mst.edu", "us.pool.ntp.org", "time.nist.gov", "pool.ntp.org"
#define NTP_SERVER_PRIMARY "0.ntp.mst.edu"

// Set Metric or Imperial mode for local sensor
const boolean IS_METRIC = false;

// Enable or disable OpenWeatherMap or Wunderground
#define OWMClient

#ifdef OWMClient
// OpenWeatherMap Settings
// Sign up here to get an API key:
// https://docs.thingpulse.com/how-tos/openweathermap-key/
const boolean OWM_IS_METRIC = false;
String OPEN_WEATHER_MAP_APP_ID = "XXXXXXX";
String OPEN_WEATHER_MAP_LOCATION = "Rolla,US"; //4406282

// Adjust according to your language
const String WDAY_NAMES[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
const String MONTH_NAMES[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

// Pick a language code from this list:
// Arabic - ar, Bulgarian - bg, Catalan - ca, Czech - cz, German - de, Greek - el,
// English - en, Persian (Farsi) - fa, Finnish - fi, French - fr, Galician - gl,
// Croatian - hr, Hungarian - hu, Italian - it, Japanese - ja, Korean - kr,
// Latvian - la, Lithuanian - lt, Macedonian - mk, Dutch - nl, Polish - pl,
// Portuguese - pt, Romanian - ro, Russian - ru, Swedish - se, Slovak - sk,
// Slovenian - sl, Spanish - es, Turkish - tr, Ukrainian - ua, Vietnamese - vi,
// Chinese Simplified - zh_cn, Chinese Traditional - zh_tw.

String OPEN_WEATHER_MAP_LANGUAGE = "en";


#else
// Wunderground Settings
const boolean WU_IS_METRIC = false;
const String WUNDERGRROUND_API_KEY = "XXXXXXX";
const String WUNDERGRROUND_LANGUAGE = "EN"; // as per https://www.wunderground.com/weather/api/d/docs?d=resources/country-to-iso-matching
const String WUNDERGROUND_COUNTRY = "MO";
const String WUNDERGROUND_CITY = "Rolla";
#endif



// Thingspeak Settings
// Enable or disable Thingspeak data fetch (if you have another sensor that you want to fetch data from)
//#define TSClient
#ifdef TSClient
const String THINGSPEAK_CHANNEL_ID = "XXXXXXX";
const String THINGSPEAK_API_READ_KEY = "XXXXXXX";
#endif

#ifdef spiOLED
SSD1306Spi display(OLED_RESET, OLED_DC, OLED_CS);  // SPI OLED
#endif
#ifdef i2cOLED
SSD1306Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);  // I2C OLED
#endif


OLEDDisplayUi   ui( &display );

// Setup simpleDSTadjust Library rules
simpleDSTadjust dstAdjusted(StartRule, EndRule);

/***************************
 * End Settings
 **************************/
 
