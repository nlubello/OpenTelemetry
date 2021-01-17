
#include "OT_setup.h"
#include <Arduino.h> /* we need arduino functions to implement this */

// Init global variables
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

#ifdef ESP82
// esp8266 / NodeMCU software serial ports
SoftwareSerial mySerial(D2, D1); // RX, TX
#else
SoftwareSerial mySerial(27,26); // RX, TX
#endif

Nextion *next = Nextion::GetInstance(Serial); 

NexPage p0(next, 0, "page0");

NexText tTime = NexText(next, 0, 1, "t0", &p0);
NexText tTimeGap = NexText(next, 0, 2, "t1", &p0);
NexText tTimeBest = NexText(next, 0, 3, "t2", &p0);
NexText tSpeed = NexText(next, 0, 5, "t4", &p0);
NexText tTempEng = NexText(next, 0, 7, "t6", &p0);
NexText tTempExh = NexText(next, 0, 8, "t7", &p0);
NexText tRPM = NexText(next, 0, 6, "t5", &p0);
NexText tLap = NexText(next, 0, 4, "t3", &p0);
// Data points
Point sensorWifi("wifi_status");
Point sensorExhTemp("exhaust_temp");
Point sensorAcc("accelerometer");
Point sensorGps("gps");
Point sensorBattery("battery");
Point sensorCommands("commands");
// Buffers
File bufferFS;
// Sensors
MAX6675 thermocouple(THERMO_CLK, THERMO_CS, THERMO_DO);
MMA8452Q accelerometer(0x1C);
SFE_UBLOX_GPS myGPS;

// Global variables
bool raceMode = true;
unsigned long lastLapTime = 0;
unsigned long bestLapTime = 0;

void telemetrySetup() {
  initGlobals();

  pinMode (LED_BUILTIN, OUTPUT);

  // Launch SPIFFS file system  
  if(!SPIFFS.begin()){ 
    Debug.println("An Error has occurred while mounting SPIFFS");  
  }
  bufferFS = SPIFFS.open(BUFFER_FN, "a");
  if(!bufferFS){ 
    Debug.println("Failed to open Buffer file for reading");
    bool formatted = SPIFFS.format();
    if ( formatted ) {
      Debug.println("SPIFFS formatted successfully");
      bufferFS = SPIFFS.open(BUFFER_FN, "a");
      if(!bufferFS){
        Debug.println("Failed to open file for reading after format!"); 
        return; 
      }
    } else {
      Debug.println("Error formatting");
      return; 
    }
  }

  initGPS();

  myGPS.setUART1Output(COM_TYPE_UBX); //Set the UART port to output UBX only
  myGPS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  myGPS.saveConfiguration(); //Save the current settings to flash and BBR

  myGPS.setNavigationFrequency(10);           //Set output to 10 times a second
  byte rate = myGPS.getNavigationFrequency(); //Get the update rate of this module
  Debug.print("Current update rate:");
  Debug.println(rate);

  // Connect WiFi
  Debug.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  int i = 0;
  while (wifiMulti.run() != WL_CONNECTED) {
    Debug.print(".");
    delay(100);
    i++;
    if(i>1) break; // Timeout 10s
  }

  Debug.begin(DEVICE_ID);
  Debug.setResetCmdEnabled(true); // Enable the reset command


  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(DEVICE_ID);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Debug.println("Start updating " + type);
      raceMode = false;
    })
    .onEnd([]() {
      Debug.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Debug.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Debug.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Debug.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Debug.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Debug.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Debug.println("Receive Failed");
      else if (error == OTA_END_ERROR) Debug.println("End Failed");
    });

  ArduinoOTA.begin();
  Debug.println();

  // Set InfluxDB 1 authentication params
  client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);

  // Add constant tags - only once
  sensorWifi.addTag("device", DEVICE_ID);
  sensorWifi.addTag("SSID", WiFi.SSID());

  sensorExhTemp.addTag("device", DEVICE_ID);
  sensorAcc.addTag("device", DEVICE_ID);
  sensorGps.addTag("device", DEVICE_ID);
  sensorBattery.addTag("device", DEVICE_ID);
  sensorCommands.addTag("device", DEVICE_ID);
  
  // Accurate time is necessary for certificate validation and writing in batches
  // Syncing progress and the time will be printed to Serial.
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Check server connection
  if (client.validateConnection()) {
    Debug.print("Connected to InfluxDB: ");
    Debug.println(client.getServerUrl());
  } else {
    Debug.print("InfluxDB connection failed: ");
    Debug.println(client.getLastErrorMessage());
  }
  
  // Enable messages batching and retry buffer
  client.setWriteOptions(WriteOptions().writePrecision(WRITE_PRECISION).batchSize(MAX_BATCH_SIZE).bufferSize(WRITE_BUFFER_SIZE));
  client.setHTTPOptions(HTTPOptions().connectionReuse(true).httpReadTimeout(1000));

  // initialize the accelerometer with default paramenters (8G and 400 Hz)
  accelerometer.init();

  // Initialize Nextion connection wiht selected baud in this case 19200
  if(!next->nexInit(115200)) {
    Debug.println("nextion init fails"); 
  } else {
    Debug.println("nextion init OK"); 
  }

  xTaskCreatePinnedToCore(
                    wifiConnection,          /* Task function. */
                    "wifi",        /* String with name of task. */
                    10000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    3,                /* Priority of the task. */
                    NULL,            /* Task handle. */
                    0); /* Core where the task should run */

  xTaskCreatePinnedToCore(
                    writeSensors,          /* Task function. */
                    "writeSensors",        /* String with name of task. */
                    10000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    2,                /* Priority of the task. */
                    NULL,            /* Task handle. */
                    1); /* Core where the task should run */

  
}

void initGPS(){
    //Assume that the U-Blox GPS is running at 9600 baud (the default) or at 38400 baud.
    //Loop until we're in sync and then ensure it's at 38400 baud.
    do {
        Debug.println("GPS: trying 38400 baud");
        Serial2.begin(38400, SERIAL_8N1, RXD2, TXD2);
        if (myGPS.begin(Serial2) == true) break;

        delay(100);
        Debug.println("GPS: trying 9600 baud");
        Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
        if (myGPS.begin(Serial2) == true) {
            Debug.println("GPS: connected at 9600 baud, switching to 38400");
            myGPS.setSerialRate(38400);
            delay(100);
        } else {
            //myGPS.factoryReset();
            delay(2000); //Wait a bit before trying again to limit the Serial output
        }
    } while(1);
    Debug.println("GPS serial connected");
}


void writeSensors(void * parameter){
  while(true){

    //Wait 100ms
    //Debug.println("TASK SENSOR - Wait 100ms");
    delay(100);

    if(!raceMode){
      //Debug.println("Race mode disabled!");
    } else {
      //Debug.println("Race mode enabled!");

      // Set identical time for the whole network scan
      unsigned long long tnow = now() * pow(1000LL,3);
      //printLocalTime();
      
      // read the values of the device
      accelerometer.read();

      if (myGPS.getPVT())
      {
        // Adjust time from GPS
        setTime(myGPS.getHour(), myGPS.getMinute(), myGPS.getSecond(), myGPS.getDay(), myGPS.getMonth(), myGPS.getYear());
        tnow = now() * pow(1000LL,3) + myGPS.getNanosecond();

        byte SIV = myGPS.getSIV();
        Debug.print(F("SIV: "));
        Debug.print(SIV);

        Debug.print(" GPS TIME: ");
        Debug.print(myGPS.getYear());
        Debug.print("-");
        Debug.print(myGPS.getMonth());
        Debug.print("-");
        Debug.print(myGPS.getDay());
        Debug.print(" ");
        Debug.print(myGPS.getHour());
        Debug.print(":");
        Debug.print(myGPS.getMinute());
        Debug.print(":");
        Debug.print(myGPS.getSecond());
        Debug.print(".");
        Debug.print(myGPS.getNanosecond());
        Debug.println();
        
        sensorGps.clearFields();
        sensorGps.addField("lat", myGPS.getLatitude()); // Latitude (degrees * 10^-7)
        sensorGps.addField("long", myGPS.getLongitude()); // Longitude (degrees * 10^-7)
        sensorGps.addField("alt", myGPS.getAltitudeMSL()); // Number of mm above mean sea level (mm)
        sensorGps.addField("SIV", myGPS.getSIV()); //Number of satellites used in position solution
        sensorGps.addField("speed", myGPS.getGroundSpeed()); //Number of satellites used in position solution
        sensorGps.setTime(tnow);  //set the time
        myGPS.flushPVT();
        if (!client.writePoint(sensorGps)) {
          Debug.print("InfluxDB write failed: ");
          Debug.println(client.getLastErrorMessage());
        }
      }

      // Store measured value into point
      sensorWifi.clearFields();
      sensorWifi.addField("rssi", WiFi.RSSI()); // Report RSSI of currently connected network
      sensorWifi.setTime(tnow);  //set the time
      
      sensorExhTemp.clearFields();
      sensorExhTemp.addField("temperature", thermocouple.readCelsius()); // Report Exhaust temperature
      sensorExhTemp.setTime(tnow);  //set the time
      

      sensorAcc.clearFields();
      sensorAcc.addField("x", accelerometer.x); // X axis acceleration
      sensorAcc.addField("raw_x", accelerometer.raw_x); // X axis acceleration
      sensorAcc.addField("y", accelerometer.y); // X axis acceleration
      sensorAcc.addField("raw_y", accelerometer.raw_y); // X axis acceleration
      sensorAcc.addField("z", accelerometer.z); // X axis acceleration
      sensorAcc.addField("raw_z", accelerometer.raw_z); // X axis acceleration
      sensorAcc.setTime(tnow);  //set the time

      sensorBattery.clearFields();
      int ADC_VALUE = (int)ADC_LUT[analogRead(BATTERY_PIN)];
      sensorBattery.addField("adc", ADC_VALUE);
      float voltage_value = (ADC_VALUE * 3.30f ) / (4095.00f);
      sensorBattery.addField("voltage", voltage_value);
      sensorBattery.setTime(tnow);  //set the time

      sensorCommands.clearFields();
      sensorCommands.addField("throttle_adc", analogRead(THROT_PIN));
      sensorCommands.addField("throttle", analogRead(THROT_PIN));
      sensorCommands.addField("brake_adc", analogRead(BRAKE_PIN));
      sensorCommands.addField("brake", analogRead(BRAKE_PIN));
      sensorCommands.addField("steering_adc", analogRead(STEER_PIN));
      sensorCommands.addField("steering", analogRead(STEER_PIN));
      sensorCommands.setTime(tnow);  //set the time

      // Write point
      writeInflux(sensorWifi);
      writeInflux(sensorExhTemp);
      writeInflux(sensorAcc);
      writeInflux(sensorBattery);
      writeInflux(sensorCommands);
    }

    
  }
}

void writeInflux(Point sensor){
  if (!client.writePoint(sensor)) {
    Debug.print("InfluxDB write failed: ");
    Debug.println(client.getLastErrorMessage());
  }
}

void localDisplay(){
  //while(true){
    // Update display
    char tempBuffer[32] = "";

    dtostrf(thermocouple.readCelsius(), 4, 1, tempBuffer);
    tTempExh.setText(tempBuffer);
    //Debug.println("Temperature : %s", thermocouple.readCelsius());

    memset(tempBuffer, 0, sizeof(tempBuffer));
    snprintf(tempBuffer, sizeof(tempBuffer), "%d", myGPS.getGroundSpeed());
    tSpeed.setText(tempBuffer);

    memset(tempBuffer, 0, sizeof(tempBuffer));
    int millisec, minutes, seconds;
    int tseconds;
    unsigned long times;
    times = millis()-lastLapTime;
    millisec  = times % 1000;
    tseconds = times / 1000;
    minutes = tseconds / 60;
    seconds = tseconds % 60;
    snprintf(tempBuffer, sizeof(tempBuffer), "%d.%02d.%03d ", minutes, seconds, millisec);
    tTime.setText(tempBuffer);
  //}
  
}

void wifiConnection(void * parameter){
  while(true){
    
    // If no Wifi signal, try to reconnect it
    /*if ((WiFi.RSSI() == 0) && (wifiMulti.run(100) != WL_CONNECTED))
      Debug.println("Wifi connection lost");*/
    
    //Debug.println("TASK WIFI - Wait 1000ms");
    delay(1000);

  }
}