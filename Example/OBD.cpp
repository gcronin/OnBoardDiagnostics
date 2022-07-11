#include "OBD.h"

// constructor
OBD::OBD() {
  _serialConnection = NULL;
}

void OBD::init(SoftwareSerial *_serial) {
  _serialConnection = _serial;
}

int OBD::available() {
  _serialConnection->available();
}

void OBD::flush() {
  _serialConnection->flush();
}

int OBD::read() {
  _serialConnection->read();
}

int OBD::peek() {
  _serialConnection->peek();
}

int OBD::println(const String &s) {
  _serialConnection->println(s);
}

bool OBD::reset() {
  delay(2000);
  println("atz");
  char endCharacter = '>';
  char inChar=0;
  while(inChar != endCharacter){
    if(available() > 0){
      inChar=read();
    }
  }
}

void OBD::emptyRXBuffer() {
  while(available()>0) {
    read();   
  }
}

int OBD::getDataIndexInRxArray(int responseLength, char *expectedResponse) {
  int arrayIndex = 0;
  int matchCount=0;
  for(int i=0; i<(rxBufferLength-responseLength); i++) {
    for(int j=0; j<responseLength; j++) {
      if( rxData[i+j] == expectedResponse[j] )  matchCount++; 
    }
    if(matchCount == responseLength) return (arrayIndex+responseLength);
    arrayIndex++;
  }
  return -1;
}

boolean OBD::resetCodes() {
  flush();
  emptyRXBuffer();
  println(ResetCodes.commandString);
  getResponse(ResetCodes.endCharacter); getResponse(ResetCodes.endCharacter);
  char expResponse[2] = {'4', '4'};
  if(getDataIndexInRxArray(2, &expResponse[0]) != -1)  return true; 
  else return false;
}

void OBD::getResponse(char endCharacter){
  char inChar=0;
  int loopCount = 0;
  char rxIndex=0;

  //Keep reading characters until we get the end character
  while(inChar != endCharacter){
    // break out if stuck or to prevent buffer overrun
    loopCount++;
    if(loopCount > 50 || rxIndex > (rxBufferLength-1)) {
      break;
    }

    //If a character comes in on the serial port, we need to act on it.
    else if(this->available() > 0){
      //Start by checking if we've received the end character
      if(this->peek() == endCharacter){
        //Clear the Serial buffer
        inChar=this->read();
        //Put the end of string character on our data string
        rxData[rxIndex]='\0';
      }
      //Now check if we've received a space character (' ').
      else if(this->peek() == ' '){
        //Clear the space without saving
        inChar=this->read();
      }
      //If we didn't get the end character or a space, just add the new character to the string.
      else{
        //Get the new character from the Serial port.
        inChar = this->read();
        //Add the new character to the string, and increment the index variable.
        rxData[rxIndex++]=inChar;
      }
    }
  }
}

//  won't work for voltage or codes
int OBD::getOBDData(PID _PID) {
  char expResponse[5];
  
  //request data from OBD
  println(_PID.commandString);

  //first response repeats sent data for OBD commands
  //second response is what we want.
  getResponse(_PID.endCharacter);
  getResponse(_PID.endCharacter);

  //setup expected response code... same as PID with '4' at beginning
  _PID.commandString.toCharArray(expResponse, 5);
  expResponse[0] = '4';

  // find expected response code in received response...compare 4 digits 
  int syncLocation = getDataIndexInRxArray(4, &expResponse[0]);
  if(syncLocation != -1) {
    return _PID.parseFunction(&rxData[0], syncLocation);
  }
  else return -1;
}
