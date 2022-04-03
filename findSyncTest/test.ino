char expResponse[5] = "410C";
char rxData[20] = " 010410C0CB5 C0C6D";
char rxIndex=0;
char test[5];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println(findSync());
}

void loop() {
  // put your main code here, to run repeatedly:

}

int findSync(void) {
  int location = 0;
  int k=0;
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
    if(k == 4) return location;
    location++;
  }
  return -1;
}
