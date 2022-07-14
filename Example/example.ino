#include <SoftwareSerial.h>
#include "OBD.h"

SoftwareSerial OBD_Uart(8, 9); // RX, TX
OBD OBD;

void setup() {
  OBD.init(&OBD_Uart, &Serial);
  OBD.reset();
  OBD.setDebugOn();
}

void loop() {
  OBD.getOBDData(RPMs);
  delay(300);

}
