/**
 * OpenTelemtry Source Files
 **/
//#include <OT_config.h>
#include <OT_setup.h>

void setup(){
  telemetrySetup();
}

void loop() {

  ArduinoOTA.handle();
  // Remote debug over WiFi
  Debug.handle();

  if(client.isBufferFull()){
    Debug.println("Buffer FULL!");
  }
  digitalWrite(LED_BUILTIN, !client.isBufferFull());

  localDisplay();

  //Wait 100ms
  //Debug.println("Wait 100ms");
  delay(25);
}
