//------------------------------------------------------------------------------------------------------------
// Use JeeNode as Power Consumption Sensor based on power meter LED blink frequency, including Vcc measurement
//------------------------------------------------------------------------------------------------------------

#include <JeeLib.h> // https://github.com/jcw/jeelib
#include "pins_arduino.h"

#define MEASUREMENT_INTERVAL 1000 // ms
#define RMF12B_GROUP 33
#define RMF12B_NODE 24

ISR(WDT_vect) {
  Sleepy::watchdogEvent();  // interrupt handler for JeeLabs Sleepy power saving
}

// Payload structure
typedef struct {
  int powerConsumption;     // Power consumption [W]
} Payload;

Payload measurement;

void setup() {

  rf12_initialize(RMF12B_NODE, RF12_868MHZ, RMF12B_GROUP);
  measurement.powerConsumption = 999; // TESTING

}

void loop() {

  // measure power consumption
  measurement.powerConsumption++; // TESTING
  
  rf12_sendNow(0, &measurement, sizeof measurement);
  rf12_sendWait(2);

  delay(MEASUREMENT_INTERVAL); //JeeLabs power save function: enter low power
}


