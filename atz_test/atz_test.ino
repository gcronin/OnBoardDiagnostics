#include <SoftwareSerial.h>

SoftwareSerial OBD(8, 9); // RX, TX

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  OBD.begin(9600);
  delay(2000);
  OBD.println("atz");
  char endCharacter = '>';
  char inChar=0;
  while(inChar != endCharacter){
    if(OBD.available() > 0){
      inChar=OBD.read();
      Serial.print(inChar);
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("done");
}
