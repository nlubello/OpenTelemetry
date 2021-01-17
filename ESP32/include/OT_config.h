#ifndef OT_CONFIG_H /* include guards */
#define OT_CONFIG_H
#include <Arduino.h>

#include <SPIFFS.h>
#include <TimeLib.h>
#include <InfluxDbClient.h>
#include <max6675.h>
#include <RoboCore_MMA8452Q.h>
#include <SparkFun_Ublox_Arduino_Library.h> //http://librarymanager/All#SparkFun_Ublox_GPS
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "calibration.h"
#include <SoftwareSerial.h>
#include <Nextion.h>
#include <RemoteDebug.h>  //https://github.com/JoaoLopesF/RemoteDebug

#if defined(ESP32)
#include <WiFiMulti.h>
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
#endif

// WiFi AP SSID
#define WIFI_SSID "CasaLube"
// WiFi password
#define WIFI_PASSWORD "Nlubello.eu1991"

// InfluxDB  server url. Don't use localhost, always server name or ip address.
// E.g. http://192.168.1.48:8086 (In InfluxDB 2 UI -> Load Data -> Client Libraries), 
#define INFLUXDB_URL "http://172.24.1.1:8086"
// InfluxDB 2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)
//#define INFLUXDB_TOKEN "toked-id"
// InfluxDB 2 organization id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )
//#define INFLUXDB_ORG "org"

// InfluxDB 2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)
//#define INFLUXDB_BUCKET "bucket"
// InfluxDB v1 database name 
#define INFLUXDB_DB_NAME "telemetry"
#define INFLUXDB_USER "telemetry"
#define INFLUXDB_PASSWORD "Pr0secc0"

#define DEVICE_ID "Telemetry_ESP32_0001"

#define WRITE_PRECISION WritePrecision::NS
#define MAX_BATCH_SIZE 10
#define WRITE_BUFFER_SIZE 255

#define RXD2 16
#define TXD2 17


#define THERMO_DO 19
#define THERMO_CS 23
#define THERMO_CLK 18
#define LED_BUILTIN 5

#define BUFFER_FN "/bufferInflux.txt"

#define BATTERY_PIN 35
#define THROT_PIN 39
#define BRAKE_PIN 36
#define STEER_PIN 32


#endif /* OT_CONFIG_H */