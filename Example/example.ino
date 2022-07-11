#include <SoftwareSerial.h>
#include "OBD.h"

SoftwareSerial OBD_Uart(8, 9); // RX, TX
OBD OBD;

void setup() {
  //OBD_Uart.begin(9600);
  //OBD.init(&OBD_Uart);
  //OBD.reset();
  Serial.begin(9600);
  Serial.println(PIDs[0].commandString);
}

void loop() {


}
