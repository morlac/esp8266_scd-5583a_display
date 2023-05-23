/**
 * 
 * partly inspired by and copied from: https://fipsok.de/Projekt/esp8266-ntp-zeit
 */

#include <FS.h> // MUST be first as otherwise things get screwd up

#include <Arduino.h>
#include <Streaming.h>

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#define SERIAL_BAUDRATE 115200


//#define MQTT_MAX_PACKET_SIZE 512 // https://pubsubclient.knolleary.net/api#configoptions

#include <ArduinoHA.h>
#include <ESP8266WiFi.h>

WiFiClient client;
HADevice device;
char device_name[20] = {0};
HAMqtt mqtt(client, device);

HASensorNumber HA_Temp("T", HASensorNumber::PrecisionP2);
HASensorNumber HA_Humidity("H", HASensorNumber::PrecisionP3);
HASensorNumber HA_Pressure("P", HASensorNumber::PrecisionP0);

// TODO: add switches to en/disable reporting of sensordata

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

#include <LittleFS.h>
FS* filesystem =      &LittleFS;
#define FileFS        LittleFS
#define FS_Name       "LittleFS"

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

const char* configFileName = "/config.json";

// From v1.1.1
// You only need to format the filesystem once
//#define FORMAT_FILESYSTEM       true
#define FORMAT_FILESYSTEM         false

char mqtt_server[40] = {0};
char mqtt_port[6] = "1883";
char mqtt_user[20] = {0};
char mqtt_pass[20] = {0};

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial << F("Should save config") << endl;

  shouldSaveConfig = true;
}

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
void onMqttConnected() {
  Serial << ("Connected to the broker!") << endl;

    // You can subscribe to custom topic if you need
    //mqtt.subscribe("myCustomTopic");
}

/**
 * 
 */
void setup(void) {
  Serial.begin(SERIAL_BAUDRATE);
  
  {
    scd1.clearMatrix();
    scd1.setBrightness(BRIGHTNESS);

    scd2.clearMatrix();
    scd2.setBrightness(BRIGHTNESS);
  }

  if (FORMAT_FILESYSTEM) {
    FileFS.format();
  }

  //read configuration from FS json
  Serial << F("mounting FS...") << endl;

  if (FileFS.begin()) {
    Serial << F("mounted file system") << endl;
    
    if (FileFS.exists(configFileName)) {
      //file exists, reading and loading
      Serial << F("reading config file") << endl;
      
      File configFile = FileFS.open(configFileName, "r");
      
      if (configFile) {
        Serial << F("opened config file") << endl;
        size_t size = configFile.size();

        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

#if ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
#endif
          Serial << endl << F("parsed json") << endl;

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_pass, json["mqtt_pass"]);
        } else {
          Serial << F("failed to load json config") << endl;
        }

        configFile.close();
      }
    }
  } else {
    Serial << F("failed to mount FS") << endl;
  }

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

    // The extra parameters to be configured (can be either global or just in the setup)
    // After connecting, parameter.getValue() will get you the configured value
    // id/name placeholder/prompt default length
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
    WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 20);
    WiFiManagerParameter custom_mqtt_pass("pass", "mqtt pass", mqtt_pass, 20);

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    //add all your parameters here
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_mqtt_user);
    wifiManager.addParameter(&custom_mqtt_pass);
  
    /*{
      //reset settings - for testing
      wifiManager.resetSettings();
      ESP.eraseConfig();
    } */

    wifiManager.setTimeout(300);

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "SCD5583"
    //and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("SCD5583");
    //or use this for auto generated name ESP + ChipID

    //read updated parameters
    strncpy(mqtt_server, custom_mqtt_server.getValue(), sizeof(mqtt_server) - 1);
    strncpy(mqtt_port, custom_mqtt_port.getValue(), sizeof(mqtt_port) - 1);
    strncpy(mqtt_user, custom_mqtt_user.getValue(), sizeof(mqtt_user) - 1);
    strncpy(mqtt_pass, custom_mqtt_pass.getValue(), sizeof(mqtt_pass) - 1);
  
    Serial << F("The values in the file are: ") << endl;
    Serial << F("\tmqtt_server : [") << String(mqtt_server) << F("]") << endl;
    Serial << F("\tmqtt_port   : [") << String(mqtt_port) << F("]") << endl;
    Serial << F("\tmqtt_user   : [") << String(mqtt_user) << F("]") << endl;
    Serial << F("\tmqtt_pass   : [") << String(mqtt_pass) << F("]") << endl;    

    //save the custom parameters to FS
    if (shouldSaveConfig) {
      Serial << F("saving config") << endl;

#if ARDUINOJSON_VERSION_MAJOR >= 6
      DynamicJsonDocument json(1024);
#else
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
#endif

      json["mqtt_server"] = mqtt_server;
      json["mqtt_port"] = mqtt_port;
      json["mqtt_user"] = mqtt_user;
      json["mqtt_pass"] = mqtt_pass;

      File configFile = FileFS.open(configFileName, "w");
    
      if (!configFile) {
        Serial << F("failed to open config file for writing") << endl;
    }

#if ARDUINOJSON_VERSION_MAJOR >= 6
      serializeJson(json, Serial);
      serializeJson(json, configFile);
#else
      json.printTo(Serial);
      json.printTo(configFile);
#endif
      configFile.close();
      //end save
    }

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

    WiFi.mode(WIFI_STA);
  }

  bool timeSync = getNtpServer();

  scd1.writeLine((char*) "NTP-Sync");
  scd2.writeLine((char*) (timeSync ? " success" : " failed!"));

  SCD5583_fadeout(scd1, 2000);
  SCD5583_fadeout(scd2, 2000);

  scd1.setBrightness(BRIGHTNESS);
  scd2.setBrightness(BRIGHTNESS);

  { // start HomeAssistant init
    sprintf(device_name, "SCD5583_%d", ESP.getChipId());

    byte mac[WL_MAC_ADDR_LENGTH];
    WiFi.macAddress(mac);

    device.setName(device_name);
    device.setUniqueId(mac, sizeof(mac));
    device.setSoftwareVersion("1.1.0");
    device.setManufacturer("Morlac");
    device.setModel("ESP8266 SCD5583-Display");

    HA_Temp.setName("Temperature");
    HA_Temp.setUnitOfMeasurement("°C");
    HA_Temp.setDeviceClass("temperature");

    HA_Humidity.setName("Humidity");
    HA_Humidity.setUnitOfMeasurement("% rH");
    HA_Humidity.setDeviceClass("humidity");

    HA_Pressure.setName("Pressure");
    HA_Pressure.setUnitOfMeasurement("Pa");
    HA_Pressure.setDeviceClass("pressure");
    
    device.enableLastWill();
    
    mqtt.onConnected(onMqttConnected);
    // TODO: setup onMessage();

    mqtt.begin(mqtt_server, atoi(mqtt_port), mqtt_user, mqtt_pass);
  }
}

/**
 * 
 */
void new_loop(void) {
  char display_buf[9] = {0};

  static time_t lastsec {0};
  time_t now = time(&now);

  localtime_r(&now, &tm);

  if (tm.tm_sec != lastsec) {
    mqtt.loop();

    int32_t temperature = bme.readFixedTempC();
    uint32_t humidity = bme.readFixedHumidity();
    uint32_t barometric_pressure = bme.readFixedPressure();

    Serial << F("T: [") << temperature << F("] [") << (temperature / 100) << "." << (temperature % 100) / 10 << F("]") << endl;
    Serial << F("H: [") << humidity    << F("] [") << (humidity / 1000) << "." << (humidity % 1000) / 100 << F("]") << endl;
    Serial << F("P: [") << barometric_pressure << " Pa]" << endl; 
  
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

    if (tm.tm_sec == 0) {
      Serial << F("sending data via MQTT") << endl;

      HA_Temp.setValue((temperature / 100.0f), true);
      HA_Humidity.setValue(humidity / 1000.0f, true);
      HA_Pressure.setValue(barometric_pressure, true);
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

