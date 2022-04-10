int numModes = 5;
volatile int mode = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), increment, LOW);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(200);
    Serial.println(mode);
 
}

void increment()
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
