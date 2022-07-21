
#include <SoftwareSerial.h>
#include "OBD.h"

SoftwareSerial OBD_Uart(8, 9); // RX, TX
OBD OBD;

void setup() {
  OBD.init(&OBD_Uart, &Serial);
  OBD.setDebugOn();
  OBD.reset();
}

void loop() {
  OBD.getOBDData(RPMs);
  delay(1000);
  OBD.getOBDData(Load);
  delay(1000);
  OBD.getOBDData(EngineTemp);
  delay(1000);
  OBD.getVoltage();
  delay(1000);  
}
