typedef int (*parseFunction) (char *data, int index);

typedef struct{
  String commandString;
  String displayText;
  char endCharacter;
  parseFunction parseFunction;
} PID;

int parseLoad(char *data, int index) {
  int temp = strtol(&data[index],0,16)*100/255;
  return (abs(temp) > 100) ? -1 : temp;
}

parseFunction fparseLoad = parseLoad;


PID Load = {"0104", "Load%", '\r', fparseLoad};
PID EngineTemp = {"0105", "Eng F", '\r'};
PID RPMs = {"010C", "RPMs ", '\r'};
PID EngineCodes = {"03", "Codes", '\r'};
PID ResetCodes = {"04", "Reset", '\r'};

PID PIDs[3] = {Load, EngineTemp, RPMs};
