/*
  Jeelib "test1" extended for first DS18B20 test on JeeNode Micro v3
  
  - read T from PIN 8 = DIO2, other option would be 10 = DIO1
  - Blink LED on PIN 9 = PA1
  - send out one test packet per second.
  
  141214 PWidmayer
*/
#include <JeeLib.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into 
#define ONE_WIRE_BUS 8

// Blink LED with data package send-out
const byte LED = 9;
byte counter;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// turn the on-board LED on or off
static void led (bool on) {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, on ? 0 : 1); // inverted logic
}

void setup () {

  // JNu v3: Power up the wireless hardware
  bitSet(DDRB, 0);
  bitClear(PORTB, 0);
  delay(1000);

  // this is node 1 in net group 100 on the 868 MHz band
  rf12_initialize(1, RF12_868MHZ, 100);

  // Start up OoneWire library
  sensors.begin();
}

void loop () {
  led(true);

  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  sensors.requestTemperatures(); // Send the command to get temperatures
  
  // Temperature for the device 1 (index 0) 
  Serial.println(sensors.getTempCByIndex(0));  

  // actual packet send: broadcast to all, current counter, 1 byte long
  rf12_sendNow(0, &counter, 1);;
  rf12_sendWait(1);

  delay(1000);
  led(false);

  // increment the counter (it'll wrap from 255 to 0)
  ++counter;
  // let one second pass before sending out another packet
  delay(1000);
}


