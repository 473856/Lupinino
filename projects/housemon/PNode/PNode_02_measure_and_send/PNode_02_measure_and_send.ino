//-----------------------------------------------------------------------
// Use JeeNode to
//   - Measure Power Consumption based on power meter LED blink frequency
//   - send Power Consumption data via RFM12B
//
// Frequency counting for frequencies low enough to execute an interrupt for each cycle.
// Frequency source connected to INT1 pin (digital pin 3 on an Arduino Uno - this also works on JeeNode v6)
//
// based on http://forum.arduino.cc/index.php?topic=64219.60
//
// 150529 473856@posteo.org
//
//-----------------------------------------------------------------------

#include <JeeLib.h> // https://github.com/jcw/jeelib

#define MEASUREMENT_INTERVAL_MS 10000
#define RMF12B_GROUP 33
#define RMF12B_NODE 24

//Number of pulses per wh - found or set on the meter. 
// 10000 pulses/kWh = 10 pulses per Wh = 1/360 pulses per Ws
#define POWERMETERCONST 360

volatile unsigned long firstPulseTime;
volatile unsigned long lastPulseTime;
volatile unsigned long numPulses;
unsigned long currentMilis;
unsigned long lastMilis = 0;
double Hz;

// Payload structure
typedef struct {
    int powerConsumption;     // Power consumption [W]
} Payload;

Payload measurement;

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

void setup() {

  rf12_initialize(RMF12B_NODE, RF12_868MHZ, RMF12B_GROUP);

  Serial.begin(57600);    // this is here so that we can print the result
  attachInterrupt(1, isr, RISING);    // enable the interrupt
}

void loop() {

  currentMilis = millis();
  attachInterrupt(1, isr, RISING);    // enable the interrupt

  if (numPulses >= 3 && currentMilis - lastMilis > MEASUREMENT_INTERVAL_MS) // instead of using delay()
  {
    detachInterrupt(1);
    lastMilis = currentMilis;
    Hz =  (1000000.0 * (double)(numPulses - 2)) / (double)(lastPulseTime - firstPulseTime);
    measurement.powerConsumption = (int) (Hz * POWERMETERCONST);

    rf12_sendNow(0, &measurement, sizeof measurement);
    rf12_sendWait(2);

    numPulses = 0;
    attachInterrupt(1, isr, RISING);    // enable the interrupt

    // for debugging only
    Serial.println(Hz);
    Serial.println(measurement.powerConsumption);
  }

}


