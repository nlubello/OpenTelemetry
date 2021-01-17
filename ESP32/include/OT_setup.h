
#ifndef OT_SETUP_H /* include guards */
#define OT_SETUP_H

#include "OT_config.h"

#include "RemoteDebug.h"  //https://github.com/JoaoLopesF/RemoteDebug
extern RemoteDebug Debug;

#if defined(ESP32)
#include <WiFiMulti.h>
extern WiFiMulti wifiMulti;
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
extern ESP8266WiFiMulti wifiMulti;
#endif

// InfluxDB client instance
//InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
// InfluxDB client instance for InfluxDB 1
extern InfluxDBClient client;

extern const char* ntpServer;
extern const long  gmtOffset_sec;
extern const int   daylightOffset_sec;

#ifdef ESP82
// esp8266 / NodeMCU software serial ports
extern SoftwareSerial mySerial; // RX, TX
#else
extern SoftwareSerial mySerial; // RX, TX
#endif

/*
* Declare Nextion instance
*/
extern Nextion *next; 


/*
 * Declare a main page object [nextion instance, page id:0, page name: "page0"]. 
 */
extern NexPage p0;

/*
 * Declare a text object [nextion instance, page id:0,component id:1, component name: "s0", page]. 
 */
extern NexText tTime;
extern NexText tTimeGap;
extern NexText tTimeBest;
extern NexText tSpeed;
extern NexText tTempEng;
extern NexText tTempExh;
extern NexText tRPM;
extern NexText tLap;

// Data points
extern Point sensorWifi;
extern Point sensorExhTemp;
extern Point sensorAcc;
extern Point sensorGps;
extern Point sensorBattery;
extern Point sensorCommands;

// Sensors
extern MAX6675 thermocouple;
extern MMA8452Q accelerometer;
extern SFE_UBLOX_GPS myGPS;

// Buffers
extern File bufferFS;

// Global variables
extern bool raceMode;
extern unsigned long lastLapTime;
extern unsigned long bestLapTime;

// Functions declaration
void telemetrySetup();
void initGPS();
void writeSensors(void * parameter);
void writeInflux(Point sensor);
void wifiConnection(void * parameter);
void localDisplay();

#endif /* OT_SETUP_H */