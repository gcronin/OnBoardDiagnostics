#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

SoftwareSerial OBD(10, 11); // RX, TX

//This is a character buffer that will store the data from the serial port
char rxData[20];
char rxIndex=0;
String data="010C";
char expResponse[5] = "410C";

void setup() {
  Serial.begin(9600);
  OBD.begin(9600);
  lcd.begin(16, 2);
  lcd.setCursor(0, 1);
  lcd.print("RPM: ");
}

void loop() { 
  OBD.println(data);  //request data from OBD
  getResponse();
  getResponse();
  int syncLocation = findSync();
  if(syncLocation != -1) {
    for(int i=0; i<20; i++)
    { 
      Serial.print(rxData[i]);
    }
    Serial.print(" Index: ");
    Serial.print(syncLocation);
    int vehicleRPM = strtol(&rxData[syncLocation],0,16)/4;
    if(vehicleRPM != -1) {
      Serial.print("  RPM: ");
      Serial.println(vehicleRPM);
    }
    else Serial.println("");

    lcd.setCursor(5, 1);
    //Clear the old RPM data, and then move the cursor position back.
    lcd.print("           ");
    lcd.setCursor(5, 1);
    lcd.print(vehicleRPM);
  }
  delay(200);
}

//The getResponse function collects incoming data from the UART into the rxData buffer
// and only exits when a carriage return character is seen. Once the carriage return
// string is detected, the rxData buffer is null terminated (so we can treat it as a string)
// and the rxData index is reset to 0 so that the next string can be copied.
void getResponse(int timeout){
  char inChar=0;
  //Keep reading characters until we get a carriage return
  while(inChar != '\r'){
    // prevent buffer overflow
    if(rxIndex > 19) {
      rxIndex = 0;
      break;
    }
    //If a character comes in on the serial port, we need to act on it.
    else if(OBD.available() > 0){
      //Start by checking if we've received the end of message character ('\r').
      if(OBD.peek() == '\r'){
        //Clear the Serial buffer
        inChar=OBD.read();
        //Put the end of string character on our data string
        rxData[rxIndex]='\0';
        //Reset the buffer index so that the next character goes back at the beginning of the string.
        rxIndex=0;
      }
      //Now check if we've received a space character (' ').
      else if(OBD.peek() == ' '){
        //Clear the Serial buffer
        inChar=OBD.read();
      }
      //If we didn't get the end of message character or a space, just add the new character to the string.
      else{
        //Get the new character from the Serial port.
        inChar = OBD.read();
        //Add the new character to the string, and increment the index variable.
        rxData[rxIndex++]=inChar;
      }
    }
  }
}

void getResponse(void){
  getResponse(100);
}

//Look for the expected response in an array of characters.  Return the
//array index just after the expected response or -1 if not found.
int findSync(void) {
  int location = 0;
  int k=0;
  char test[5]; 
  for(int i=0; i<16; i++) {
    for(int j=0; j<4; j++) {
      test[j] = rxData[i+j];
    }
    k=0;
    for(int j=0; j<4; j++) {
      if ( test[j] == expResponse[j] )
      {
        k=k+1;
      }
    }
    if(k == 4) return (location+4);
    location++;
  }
  return -1;
}
