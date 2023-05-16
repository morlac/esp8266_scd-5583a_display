/**
 * 
 * partly inspired by and copied from: https://fipsok.de/Projekt/esp8266-ntp-zeit
 */
#include <Arduino.h>
#include <Streaming.h>

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#define SERIAL_BAUDRATE 115200

#include <SCD5583.hpp>

#define LOAD_PIN_1 D7
#define LOAD_PIN_2 D8
#define DATA_PIN   D5
#define CLK_PIN    D6

#define BRIGHTNESS   2

SCD5583 scd1(LOAD_PIN_1, DATA_PIN, CLK_PIN);
SCD5583 scd2(LOAD_PIN_2, DATA_PIN, CLK_PIN);

#include <Wire.h>
#define I2C_BME_ADDRESS 0x76
#define TINY_BME280_I2C
#include <TinyBME280.h>

tiny::BME280 bme;

#include <time.h>
struct tm tm;         // http://www.cplusplus.com/reference/ctime/tm/
const uint32_t SYNC_INTERVAL = 12;              // NTP Sync Interval in Stunden

const char* const PROGMEM NTP_SERVER[] = {"fritz.box", "de.pool.ntp.org", "at.pool.ntp.org", "ch.pool.ntp.org", "ptbtime1.ptb.de", "europe.pool.ntp.org"};
//const char* const PROGMEM DAY_NAMES[] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};
//const char* const PROGMEM DAY_SHORT[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
//const char* const PROGMEM MONTH_NAMES[] = {"Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"};
//const char* const PROGMEM MONTH_SHORT[] = {"Jan", "Feb", "Mrz", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"};

extern "C" uint8_t sntp_getreachability(uint8_t);

bool getNtpServer(bool reply = false) {
  uint32_t timeout {millis()};

  // Zeitzone einstellen https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  configTime("CET-1CEST,M3.5.0,M10.5.0/3", NTP_SERVER[0], NTP_SERVER[1], NTP_SERVER[2]);

  do {
    delay(25);

    if (millis() - timeout >= 1e3) {
      Serial.printf("Warten auf NTP-Antwort %02ld sec\n", (millis() - timeout) / 1000);
  
      delay(975);
    }

    sntp_getreachability(0) ? reply = true : sntp_getreachability(1) ? reply = true : sntp_getreachability(2) ? reply = true : false;
  } while (millis() - timeout <= 16e3 && !reply);
  
  return reply;
}

/**
 * 
 */
void SCD5583_fadeout(SCD5583 scd, uint16_t fade_duration) {
  int start = scd.getBrightness();
  
  for (int i = start; i <= 7; i++) {
    delay(fade_duration / (8 - start));

    scd.setBrightness(i);
  }

  scd.clearMatrix();
}

/**
 * 
 */
void setup() {
  {
    scd1.clearMatrix();
    scd1.setBrightness(BRIGHTNESS);

    scd2.clearMatrix();
    scd2.setBrightness(BRIGHTNESS);
  }

  Serial.begin(SERIAL_BAUDRATE);

  { // start bme init
    Wire.begin();

    scd1.writeLine((char*) "bme init");

    if (bme.beginI2C(I2C_BME_ADDRESS)) {
      Serial << "bme init done .." << endl;
      scd2.writeLine((char*) " .. done");
    } else {
      Serial << "bme init failed!" << endl;
      scd2.writeLine((char*) " failed!");
    }

    SCD5583_fadeout(scd1, 2000);
    SCD5583_fadeout(scd2, 2000);
  } // END bme init

  {
    scd1.setBrightness(BRIGHTNESS);
    scd1.writeLine((char*) "WiFi-AP:");

    scd2.setBrightness(BRIGHTNESS);
    scd2.writeLine((char*) " SCD5583");

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    //reset settings - for testing
    //wifiManager.resetSettings();

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "SCD5583"
    //and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("SCD5583");
    //or use this for auto generated name ESP + ChipID

    SCD5583_fadeout(scd1, 2000);
    SCD5583_fadeout(scd2, 2000);

    scd1.setBrightness(BRIGHTNESS);
    scd2.setBrightness(BRIGHTNESS);

    scd1.writeLine((char*) "connect ");
    scd2.writeLine((char*) " .. done");

    SCD5583_fadeout(scd1, 2000);
    SCD5583_fadeout(scd2, 2000);

    scd1.setBrightness(BRIGHTNESS);
    scd2.setBrightness(BRIGHTNESS);
  }

  bool timeSync = getNtpServer();

  scd1.writeLine((char*) "NTP-Sync");
  scd2.writeLine((char*) (timeSync ? " success" : " failed!"));

  SCD5583_fadeout(scd1, 2000);
  SCD5583_fadeout(scd2, 2000);

  scd1.setBrightness(BRIGHTNESS);
  scd2.setBrightness(BRIGHTNESS);
}

/**
 * 
 */
void new_loop(void) {
  char display_buf[9];

  static time_t lastsec {0};
  time_t now = time(&now);

  localtime_r(&now, &tm);

  if (tm.tm_sec != lastsec) {
    int32_t temperature = bme.readFixedTempC();
    uint32_t humidity = bme.readFixedHumidity();
    uint32_t barometric_pressure = bme.readFixedPressure();

    Serial << "T: [" << temperature << "] " << (temperature / 100) << "." << (temperature % 100) / 10 << endl;
    Serial << "H: [" << humidity << "] " << (humidity / 1000) << "." << (humidity % 1000) / 100 << endl;
    Serial << "P: [" << barometric_pressure << " Pa]" << endl; 
  
    lastsec = tm.tm_sec;

    strftime(display_buf, sizeof(display_buf), "%H:%M:%S", &tm);

    Serial << String(display_buf) << endl;

    scd1.writeLine(display_buf);

    int secs = tm.tm_sec;

    if ((secs >= 0) && (secs <= 14)) {
      // Temp:
      snprintf(display_buf, sizeof(display_buf), " %2d.%1d 'C", (temperature / 100), (temperature % 100) / 10);
    } else if ((secs >= 15) && (secs <= 29)) {
      // Humidity:
      snprintf(display_buf, sizeof(display_buf), "%2d.%1d %%rH", (humidity / 1000), (humidity % 1000) / 100);
    } else if ((secs >= 30) && (secs <= 44)) {
      // Barometric Pressure:
      snprintf(display_buf, sizeof(display_buf), "%5d Pa", barometric_pressure);
    } else if ((secs >= 45) && (secs <= 59)) {
      snprintf(display_buf, sizeof(display_buf), "%02d.%02d.%02d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year % 100);
    }

    scd2.writeLine(display_buf);
  }
}

/**
 * 
 */

void old_loop(void) {
  char displayBuffer[25];// = {0};

  memset(displayBuffer, ' ', 24);

  int32_t temperature = bme.readFixedTempC();
  uint32_t humidity = bme.readFixedHumidity();
  uint32_t barometric_pressure = bme.readFixedPressure();

  Serial << "T: [" << temperature << "]" << endl;
  Serial << "H: [" << humidity << "]" << endl;
  Serial << "P: [" << barometric_pressure << "]" << endl; 

  scd1.clearMatrix();
  scd2.clearMatrix();

  for (int digit = 0; digit <= 7; digit++) {
    for (int i = 0; i <= 9; i++) {
      displayBuffer[digit + 0] = '0' + i;

      scd1.writeLine(displayBuffer + 0);

      delay(250);
    }

    delay(250);
  }

  for (int b = 0; b <= 7; b++) {
    scd1.setBrightness(b);

    delay(250);
  }

  for (int b = 7; b >= 0; b--) {
    scd1.setBrightness(b);

    delay(250);
  }

  for (int digit = 0; digit <= 7; digit++) {
    for (int i = 0; i <= 9; i++) {
      displayBuffer[digit + 8] = '0' + i;

      scd2.writeLine(displayBuffer + 8);

      delay(250);
    }

    delay(250);
  }

  for (int b = 0; b <= 7; b++) {
    scd2.setBrightness(b);

    delay(500);
  }

  for (int b = 7; b >= 0; b--) {
    scd2.setBrightness(b);

    delay(250);
  }
}

void loop(void) {
  //old_loop();
  new_loop();
};

