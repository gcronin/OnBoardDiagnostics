#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
SoftwareSerial OBD(8, 9); // RX, TX

const boolean useSerial = true;
const boolean useLCD = false;

//This is a character buffer that will store the data from the serial port
char rxData[20];
char rxIndex=0;

// Setup the available OBD codes and associated names
const int numModes = 7;
volatile int mode = 2; //start with RPM
String PIDcodes[numModes] = {"0104", "0105", "010C", "010D", "010F", "0111", "atrv"};
String PIDnames[numModes] = {"Load %", "EngTemp oC", "RPM", "Speed km/hr", "AirTemp oC", "Throttle %", "Battery"};
char expResponse[5];

void setup() {
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), incrementMode, LOW);
  attachInterrupt(digitalPinToInterrupt(3), decrementMode, LOW);
  OBD.begin(9600);
  
  if(useSerial) Serial.begin(9600);
  if(useLCD) lcd.begin(16, 2);
}

void loop() { 
  //request data from OBD
  OBD.println(PIDcodes[mode]);  

  //first response repeats sent data for OBD commands (not 'at' commands)
  //second response is what we want
  if(mode!=6) getResponse();
  getResponse();

  //setup expected response code... same as PID with '4' at beginning
  PIDcodes[mode].toCharArray(expResponse, 5);
  expResponse[0] = '4';

  // find expected response code in received response unless sending 'atrv' command
  int syncLocation;
  if(mode!=6) syncLocation = findSync();
  else syncLocation = 0;
  
  // if we actually found the expected response
  if(syncLocation != -1) {
    // print full response and index of expected response
    if(useSerial) {
      for(int i=0; i<20; i++)
      { 
        Serial.print(rxData[i]);
      }
      Serial.print(" Index: ");
      Serial.print(syncLocation);
    }

    //parse response for out data
    int data;
    switch(mode) {
      case 0: // engine load, percent (max 100)
        data = strtol(&rxData[syncLocation],0,16)*100/255;
        data = (abs(data) > 100) ? -1 : data;
        break;
      case 1: // engine temp, oC (max 500)
        data = strtol(&rxData[syncLocation],0,16)-40;
        data = (abs(data) > 500) ? -1 : data;
        break;
      case 2: // RPM
        data = strtol(&rxData[syncLocation],0,16)/4;
        break;
      case 3: // speed km/hr (max 300)
        data = strtol(&rxData[syncLocation],0,16);
        data = (abs(data) > 300) ? -1 : data;
        break;
      case 4: // air temp, oC
        data = strtol(&rxData[syncLocation],0,16)-40;
        data = (abs(data) > 500) ? -1 : data;
        break;
      case 5: // throttle, percent (max 100)
        data = strtol(&rxData[syncLocation],0,16)*100/255;
        data = (abs(data) > 100) ? -1 : data;
        break;
      default:
        data = -1;
        break;        
    }

    if(data != -1) {
      if(useSerial) {
        Serial.print("  ");
        Serial.print(PIDnames[mode]);
        Serial.print("  ");
        Serial.println(data);
      }

      if(useLCD) {
        lcd.setCursor(0, 0);
        lcd.print(PIDnames[mode]);
        lcd.setCursor(0, 1);
        lcd.print("           ");
        lcd.setCursor(0, 1);
        lcd.print(data);
      }
    }
    else if(useSerial) Serial.println("");
  }
  delay(200);
}

//The getResponse function collects incoming data from the UART into the rxData buffer
// and only exits when a carriage return character is seen. Once the carriage return
// string is detected, the rxData buffer is null terminated (so we can treat it as a string)
// and the rxData index is reset to 0 so that the next string can be copied.
void getResponse(){
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

void incrementMode()
{
  //Static variable initialized only first time increment called, persists between calls.
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) 
  {
    mode = (mode+1)%numModes;
  }
  last_interrupt_time = interrupt_time;
}

void decrementMode()
{
  //Static variable initialized only first time increment called, persists between calls.
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) 
  {
    mode = (numModes-1+mode)%numModes;
  }
  last_interrupt_time = interrupt_time;
}
