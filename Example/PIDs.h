typedef int (*parseFunction) (char *data, int index);

typedef struct{
  String commandString;
  String displayText;
  char endCharacter;
  parseFunction parseFunction;
} PID;

int parseLoad(char *data, int index) {
  int load = strtol(&data[index],0,16)*100/255;
  return (abs(load) > 100) ? -1 : load;
}

int parseEngineTemp(char *data, int index) {
  int temp = strtol(&data[index],0,16)-40;
  temp = (abs(temp) > 500) ? -1 : temp;
  if(temp == -1) return -1;
  else {
    float tempC = float(temp);
    float tempF = (tempC*1.8)+32;
    return int(tempF);
  }
}

int parseRPM(char *data, int index) {
  return strtol(&data[index],0,16)/4;
}

parseFunction fparseLoad = parseLoad;
parseFunction fparseEngineTemp = parseEngineTemp;
parseFunction fparseRPM = parseRPM;


PID Load = {"0104", "Load%", '\r', fparseLoad};
PID EngineTemp = {"0105", "Eng F", '\r', fparseEngineTemp};
PID RPMs = {"010C", "RPMs ", '\r', fparseRPM};
PID EngineCodes = {"03", "Codes", '\r'};
PID ResetCodes = {"04", "Reset", '\r'};

PID PIDs[3] = {Load, EngineTemp, RPMs};
