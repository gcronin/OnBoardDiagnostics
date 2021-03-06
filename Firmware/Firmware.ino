#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <SD.h>

/* ** MOSI - pin 11  YELLOW Pin 17
   ** MISO - pin 12  BLUE  Pin 18
   ** CLK - pin 13  GREEN  Pin 19
   ** CS - pin 10  GREY Pin 16 */

const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = A1, d7 = A0;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
SoftwareSerial OBD(8, 9); // RX, TX

const boolean useSerial = false;
const boolean useLCD = true;

// Variables for reading raw and processed data from Software Serial
const uint8_t rxBufferLength = 20;
char rxData[rxBufferLength];
char rxIndex=0;
int data;  float voltage;
int syncLocation = -1;

// Setup the available OBD codes and associated names
const int numModes = 8;
volatile int modeTopLine = 2; //Top LCD Line Display
volatile int modeBottomLine = 3; //Bottom LCD Line Display
int mode;
volatile boolean modeChanged = false;
String PIDcodes[numModes] = {"0104", "0105", "010C", "010D", "010F", "0111", "atrv", "03"};
String PIDnames[numModes] = {"Load%", "Eng F", "RPMs ", "mph  ", "Air F", "Thrt%", "BattV", "Codes"};
const uint8_t battModeNum = 6;
const uint8_t codesModeNum = 7;
char expResponse[5];
boolean evenLoop = true;

//SD card variables
File SDfile;
volatile boolean logToSD = false;
boolean SDinitialized = false;
const uint8_t SDchipSelectPin = 10;

void setup() {
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), incrementModeTop, LOW);
  attachInterrupt(digitalPinToInterrupt(3), incrementModeBottom, LOW);
  OBD.begin(9600);
  if(useSerial) Serial.begin(9600);
  if(useLCD) lcd.begin(16, 2);
  resetODB();
  while(syncLocation == -1) {
    displayCodes();
    delay(500);
  }
}

void loop() { 
  OBD.flush();
  emptyRXBuffer();
  mode = evenLoop ? modeTopLine : modeBottomLine; 
  getRawData(mode);
  if(syncLocation != -1) {
    printRawData();
    parseData(mode);
    if(data != -1) {
      printParsedData(mode, evenLoop);
      logSD(mode);
    }
    else if(useSerial) Serial.println("");
  }
  readButtons();
  delay(300);
  evenLoop = !evenLoop;
}

void readButtons() {
  // log to SD
  if(!digitalRead(A2)) {
    if(useLCD) {
        lcd.setCursor(15, 1);
        lcd.print("-"); //feedback for button press
    }
    logToSD = !logToSD;
    delay(500);
    if(useLCD) {
        lcd.setCursor(15, 1);
        lcd.print(" ");
    }
  }
  // clear codes
  if(!digitalRead(A3)) {
    if(useLCD) {
        lcd.setCursor(0, 0);
        lcd.print("Erase Codes?");
        lcd.setCursor(0, 1);
        lcd.print("Y=Green N=Red");
    }
    delay(1000);  //debounce
    while(true) {
      if(!digitalRead(A3)) break;
      else if(!digitalRead(A2)) {
        resetCodes();
        break;
      }
      delay(50);
    }
    if(useLCD) {
      lcd.setCursor(0, 0);
      lcd.print("            ");
      lcd.setCursor(0, 1);
      lcd.print("             ");
    }
    delay(1000);  //debounce
  }
}

void resetCodes() {
  if(useLCD) lcd.setCursor(0, 0);
  if(useLCD) lcd.print("Erasing Codes");
  OBD.flush();
  emptyRXBuffer();
  OBD.println("04");
  getResponse(); getResponse();
  expResponse[0] = '4';  expResponse[1]='4';
  syncLocation = findSync(2);
  if(syncLocation != -1) {
    if(useLCD) lcd.setCursor(0, 1);
    if(useLCD) lcd.print("Success      ");
  }
  delay(2000);
  if(useLCD) lcd.setCursor(0, 0);
  if(useLCD) lcd.print("             ");
  if(useLCD) lcd.setCursor(0, 1);
  if(useLCD) lcd.print("             ");
}

/*!
  @brief   read Error Codes, display
*/
void displayCodes() {
  OBD.flush();
  emptyRXBuffer();
  getRawData(codesModeNum);
  printRawData();
  if(syncLocation != -1) {
    if(useLCD) {
        lcd.setCursor(0, 0);
        lcd.print("Codes:");
        lcd.setCursor(0, 1);
        //loop three times... max of 3 codes possible
        for(int j=0; j<3; j++) {
          String codeString = "";  int codeInt;
          boolean bufferOverflow = false;
          //offset is 0 first loop, 4 second loop, 8 third loop
          uint8_t indexOffset = 0;
          if(j>0) { indexOffset = (j==1) ? 4: 8; }
          //code is four characters
          for(int i=0; i<4; i++) {
            uint8_t index = syncLocation + i + indexOffset;
            if(index < rxBufferLength) {
              codeString += rxData[index];
            }
            else {
              bufferOverflow = true;
              break;
            }
          }
          codeInt = codeString.toInt();
          // error if index of rxData overflowed or toInt returned -1 (no codes are negative)
          if (codeInt<0 || bufferOverflow) {
            lcd.print("Err");
            break;
          }
          else if (codeInt>0) {
            lcd.print(codeInt);
            lcd.print(" ");
          }
          else {
            // if first code is zero than there are no codes, otherwise just stop outer loop
            if( j == 0 ) lcd.print("No Codes");
            break;
          }
        }
    }
    delay(5000);  //pause so user can read codes
    if(useLCD) {
        // clear display
        lcd.setCursor(0, 0);
        lcd.print("      ");
        lcd.setCursor(0, 1);
        lcd.print("                ");  
    }
  }
}

/*!
  @brief   print Software Serial buffer
*/
void printRawData() {
  if(useSerial) {
    for(int i=0; i<rxBufferLength; i++)
    { 
      Serial.print(rxData[i]);
    }
    Serial.print(" Index: ");
    Serial.print(syncLocation);
  }
}

/*!
  @brief   Send OBD query, get response, validate by looking for echo of query
*/
void getRawData(int _mode) {
  //request data from OBD
  OBD.println(PIDcodes[_mode]);

  //first response repeats sent data for OBD commands
  //second response is what we want.  For AT commands, only one response.
  if(_mode!=battModeNum) {  getResponse();  }
  getResponse();

  //setup expected response code... same as PID with '4' at beginning except AT commands... no change
  PIDcodes[_mode].toCharArray(expResponse, 5);
  if(_mode!=battModeNum) { expResponse[0] = '4'; }

  // find expected response code in received response...compare 4 digits for all modes except PID 03  = 2 digits 
  if(_mode!=codesModeNum) syncLocation = findSync(4);
  else syncLocation = findSync(2);
}

/*!
  @brief   parse response for our data, filter out spurious results
*/
void parseData(int _mode) {
  switch(_mode) {
    case 0: // engine load, percent (max 100)
      data = strtol(&rxData[syncLocation],0,16)*100/255;
      data = (abs(data) > 100) ? -1 : data;
      break;
    case 1: // engine temp, oF (max 500)
      data = strtol(&rxData[syncLocation],0,16)-40;
      data = (abs(data) > 500) ? -1 : celciusToFahrenheit(data);
      break;
    case 2: // RPM
      data = strtol(&rxData[syncLocation],0,16)/4;
      break;
    case 3: // speed mph (max 300 kph)
      data = strtol(&rxData[syncLocation],0,16);
      data = (abs(data) > 300) ? -1 : kphToMph(data);
      break;
    case 4: // air temp, oF
      data = strtol(&rxData[syncLocation],0,16)-40;
      data = (abs(data) > 500) ? -1 : celciusToFahrenheit(data);
      break;
    case 5: // throttle, percent (max 100)
      data = strtol(&rxData[syncLocation],0,16)*100/255;
      data = (abs(data) > 100) ? -1 : data;
      break;
    case battModeNum: // battery voltage  (8<V<20)
      String battResponse = rxData;
      battResponse = battResponse.substring(syncLocation);
      voltage = battResponse.toFloat();
      data = (voltage > 8 && voltage < 20) ? 1 : -1;
      break; 
    case codesModeNum: // actual codes
      data = 0;
      break;      
    default:
      data = -1;
      break;        
  }
}

/*!
  @brief   write data to SD card
  @note    
*/
void logSD(int _mode) {
  lcd.setCursor(15, 1);
  lcd.print(" ");
  if(logToSD) {
    if(!SDinitialized) { 
      if(SD.begin(SDchipSelectPin)) SDinitialized = true;
    }
    SDfile = SD.open("log.txt", FILE_WRITE);
    if(SDfile) {
      SDfile.print(millis());
      SDfile.print(", ");
      if(evenLoop) SDfile.print(", ");
      if(_mode == battModeNum) SDfile.print(voltage);
      else if(_mode == codesModeNum) SDfile.print(&rxData[syncLocation]);
      else SDfile.print(data);
      if(!evenLoop) SDfile.println(", ");
      else SDfile.println("");
      SDfile.close();
      if(useLCD) {
        lcd.setCursor(15, 1);
        lcd.write(255); //print a block if successfully writing
      }
    }
  }
}

/*!
  @brief   print to LCD and/or serial depending on global variables
*/
void printParsedData(int _mode, int LCDlineNum) {
  if(useSerial) {
        Serial.print("  ");
        Serial.print(PIDnames[_mode]);
        Serial.print("  ");
        if(mode == battModeNum) Serial.println(voltage);
        if(mode == codesModeNum) {
          for(int i=syncLocation; i<syncLocation+4; i++) {
            Serial.print(rxData[i]);
          }
          Serial.println("");
        }
        else Serial.println(data);
      }

      if(useLCD) {
        lcd.setCursor(0, LCDlineNum);
        lcd.print(PIDnames[_mode]);
        lcd.setCursor(6, LCDlineNum);
        lcd.print("          ");
        lcd.setCursor(6, LCDlineNum);
        if(_mode == battModeNum) lcd.print(voltage);
        if(_mode == codesModeNum) lcd.print(&rxData[syncLocation]);
        else lcd.print(data);
      }
}

/*!
  @brief   Read Software Serial buffer till end of message character received.  Discard spaces.
  @note    The getResponse function collects incoming data from the UART into the rxData buffer
            and only exits when a designated end character is seen. Once the end character
            is detected, the rxData buffer is null terminated (so we can treat it as a string)
            and the rxData index is reset to 0 so that the next string can be copied.  Break out
            if allocated buffer would overflow or an interrupt was triggered.
*/
void getResponse(){
  char inChar=0;
  char endCharacter = '\r';  //end of message character ('\r').
  int loopCount = 0;
  if(mode == battModeNum) { endCharacter = 'V'; } // in battery voltage mode, look for V
  //Keep reading characters until we get the end character
  while(inChar != endCharacter){
    loopCount++;
    if(loopCount > 25 && mode == battModeNum) {
      rxIndex = 0;
      break;
    }
    // break out of while loop if interrupt triggered
    if(modeChanged) {
      rxIndex = 0;
      modeChanged = false;
      break;
    }
    // prevent buffer overflow
    if(rxIndex > (rxBufferLength-1)) {
      rxIndex = 0;
      break;
    }
    //If a character comes in on the serial port, we need to act on it.
    else if(OBD.available() > 0){
      //Start by checking if we've received the end character
      if(OBD.peek() == endCharacter){
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
      //If we didn't get the end character or a space, just add the new character to the string.
      else{
        //Get the new character from the Serial port.
        inChar = OBD.read();
        //Add the new character to the string, and increment the index variable.
        rxData[rxIndex++]=inChar;
      }
    }
  }
}

/*!
  @brief   Look for the expected response in an array of characters.  Return the
            array index just after the expected response or -1 if not found.
*/
int findSync(int responseLength) {
  int location = 0;
  int k=0;
  char test[5]; 
  for(int i=0; i<(rxBufferLength-responseLength); i++) {
    for(int j=0; j<responseLength; j++) {
      test[j] = rxData[i+j];
    }
    k=0;
    for(int j=0; j<responseLength; j++) {
      if ( test[j] == expResponse[j] )
      {
        k=k+1;
      }
    }
    if(k == responseLength) return (location+responseLength);
    location++;
  }
  return -1;
}

/*!
  @brief   increment PID to be displayed on top line of LCD
  @note    interrupt service routine
*/
void incrementModeTop()
{
  //Static variable initialized only first time increment called, persists between calls.
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) 
  {
    modeTopLine = (modeTopLine+1)%(numModes-1);
    modeChanged = true;
  }
  last_interrupt_time = interrupt_time;
}

/*!
  @brief   increment PID to be displayed on bottom line of LCD
  @note    interrupt service routine
*/
void incrementModeBottom()
{
  //Static variable initialized only first time increment called, persists between calls.
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) 
  {
    modeBottomLine = (modeBottomLine+1)%(numModes-1);
    modeChanged = true;
  }
  last_interrupt_time = interrupt_time;
}

/*!
  @brief   clear Software Serial buffer
*/
void emptyRXBuffer() {
  while(OBD.available()>0) {
    OBD.read();   
  }
}

/*!
  @brief   reset ODB
  @note    Look for '>' character to end
*/
void resetODB() {
  if(useLCD) lcd.setCursor(0, 0);
  if(useLCD) lcd.print("Resetting");
  delay(2000);
  OBD.println("atz");
  char endCharacter = '>';
  char inChar=0;
  while(inChar != endCharacter){
    if(OBD.available() > 0){
      inChar=OBD.read();
      if(useSerial) Serial.print(inChar);
    }
  }
  if(useSerial) Serial.println("");
  if(useLCD) lcd.setCursor(0, 0);
  if(useLCD) lcd.print("         ");
}

int celciusToFahrenheit(int temp) {
  float tempC = float(temp);
  float tempF = (tempC*1.8)+32;
  return int(tempF);
}

int kphToMph(int speed) {
  float kph = float(speed);
  float mph = (kph*0.62);
  return int(mph);
}
