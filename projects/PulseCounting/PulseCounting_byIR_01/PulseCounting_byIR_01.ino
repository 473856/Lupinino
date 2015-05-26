// Frequency counter sketch, for measuring frequencies low enough to execute an interrupt for each cycle
// Connect the frequency source to the INT1 pin (digital pin 3 on an Arduino Uno)

volatile unsigned long firstPulseTime;
volatile unsigned long lastPulseTime;
volatile unsigned long numPulses;
unsigned long currentMilis;
unsigned long lastMilis = 0;
float Hz;



void isr()
{
  unsigned long now = micros();
  if (numPulses == 1) {
    firstPulseTime = now;
  }
  else {
    lastPulseTime = now;
  }
  ++numPulses;
}


void setup()
{
  Serial.begin(19200);    // this is here so that we can print the result
  attachInterrupt(1, isr, RISING);    // enable the interrupt
}




void loop()
{
  currentMilis = millis();
  attachInterrupt(1, isr, RISING);    // enable the interrupt

  if (currentMilis - lastMilis > 1000) // instead of using delay(1000)
  {
    detachInterrupt(1);
    lastMilis = currentMilis;
    Hz =  (1000000.0 * (float)(numPulses - 2)) / (float)(lastPulseTime - firstPulseTime);

    numPulses = 0;
    attachInterrupt(1, isr, RISING);    // enable the interrupt
    Serial.println(Hz);
  }
}
