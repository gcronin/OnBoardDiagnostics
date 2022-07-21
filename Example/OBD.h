#ifndef OBD_H
#define OBD_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "PIDs.h"

const uint8_t rxBufferLength = 20;

class OBD {
  
  //private:
    public:
    SoftwareSerial *_ODBConnection;
    HardwareSerial *_debug;
    char rxData[rxBufferLength];
    char rxIndex=0;
    boolean debugLevel = false;
    int println(const String &s);
    bool listen();
    virtual int available();
    virtual void flush();
    virtual int read();
    virtual int peek();
    void emptyRXBuffer();
    int getDataIndexInRxArray(int responseLength, char *expectedResponse);
    void getResponse(char endCharacter, int timeout);
    void getResponse(char endCharacter);
    

    OBD();
    void init(SoftwareSerial *_serial, HardwareSerial *_debug);
    bool reset();
    boolean resetCodes();
    int getOBDData(PID _PID);
    float getVoltage();
    void setDebugOn();
    void setDebugOff();


};

#endif
