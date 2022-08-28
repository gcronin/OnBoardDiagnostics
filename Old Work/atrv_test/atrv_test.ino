#include <SoftwareSerial.h>

boolean receivedString = false; 
boolean foundStart = false;
String Data = "";

SoftwareSerial OBD(8, 9); // RX, TX

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  OBD.begin(9600);
}

void loop() {
    OBD.flush();
    emptyRXBuffer();
    OBD.println("atrv");
    delay(100); 
  
    Serial.println(readBattery());
    delay(500);
}

float readBattery() {
  boolean receivedString = false;
  String Data = "";
  while(!receivedString) {
    if(OBD.available()) {
      char character = OBD.read();    
      if(character == 'V') { 
          receivedString = true; 

      }
      else { Data.concat(character); }
    }
  }
  Serial.print(Data);
  Serial.print("  ");
  Data=Data.substring(5);
  Serial.print(Data);
  Serial.print("  ");
  return Data.toFloat();
}

void emptyRXBuffer() {
  while(OBD.available()>0 ) {
    OBD.read();
  }
}
