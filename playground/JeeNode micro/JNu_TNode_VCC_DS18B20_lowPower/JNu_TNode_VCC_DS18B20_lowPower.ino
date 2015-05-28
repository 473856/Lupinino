//--------------------------------------------------------------------------------------
// Use JeeNode Micro as remote Temperature Sensor, including Vcc measurement
//
// Based on Funky Wireless Temperature Sensor Node:
// http://harizanov.com/2012/10/funky-as-remote-temperature-sensing-node-with-ds18b20/
//
// DS18B20 used in parasitic power mode, assumption TO92:
//    - outer pins (1/GND, 3/VDD) grounded
//    - inner pin 2 (DQ) is data IO and needs to be pulled up to power pin via 4.7k resistor
// Martin Harizanov http://harizanov.com
// GNU GPL V3
//
// adapted 130816 matt hodge
// adapted 141217 to JeeNode v3 peter widmayer
// extended 150110 to 2 x DS18B20
//
// 150524 Measurement interval extended from every 5 s to every 60 s
//   - 3 x NiMH @ 5s > 1..2 weeks, then down at 3.7 V
//--------------------------------------------------------------------------------------

#include <JeeLib.h> // https://github.com/jcw/jeelib
#include "pins_arduino.h"

#include <OneWire.h>   // http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip
#include <DallasTemperature.h>  // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_371Beta.zip

#define ONE_WIRE_BUS 8  // JNu DIO2
#define tempPower 10    // JNu DIO1: power pin

#define MEASUREMENT_INTERVAL 60000 // valid range 16-65000 ms -> JeeLabs power save duration between measurements

// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// addresses of sensors, MAX 4!!
byte allAddress [4][8];  // 8 bytes per address
#define TEMPERATURE_PRECISION 12
#define ASYNC_DELAY 750 // 9bit requires 95ms, 10bit 187ms, 11bit 375ms and 12bit resolution takes 750ms
// Be aware stated accuracy is +/- 0.5 deg Celsius - smaller relative changes might still be of interest

ISR(WDT_vect) {
  Sleepy::watchdogEvent();  // interrupt handler for JeeLabs Sleepy power saving
}

// Payload structure, assuming two DS18B20 sensors. first 2 bytes: supply voltage, next 2x2 bytes: temperatures
typedef struct {
  int volt1[3];
  //      int supplyV;	// Supply voltage
  //	int temp;	// Temperature reading
  //  	int temp2;	// Temperature 2 reading
  //  	int temp3;	// Temperature 3 reading
  //  	int temp4;	// Temperature 4 reading
} Payload;

Payload measurement;

// Power up RFM12B MOSFET - otherwise RFM12B will not initialize
static void PFETon() {
  bitSet(DDRB, 0);
  bitClear(PORTB, 0);
  rf12_initialize(22, RF12_868MHZ, 33);
  rf12_control(0xC040);
}

// Power down RFM12B MOSFET
static void PFEToff() {
  bitSet(DDRB, 0);
  bitSet(PORTB, 0);
}

int numSensors;

static void setPrescaler (uint8_t mode) {
  cli();
  CLKPR = bit(CLKPCE);
  CLKPR = mode;
  sei();
}

void setup() {

  PFETon();                  // Turn RFM12B on adjust low battery to 2.2V
  rf12_sleep(RF12_SLEEP);    // Put the RFM12 to sleep

  PRR = bit(PRTIM1); // only keep timer 0 going
  ADCSRA &= ~ bit(ADEN); // Disable the ADC
  bitSet (PRR, PRADC);   // Power down ADC
  bitClear (ACSR, ACIE); // Disable comparitor interrupts
  bitClear (ACSR, ACD);  // Power down analogue comparitor
  bitSet(PRR, PRUSI); // disable USI h/w

  pinMode(tempPower, OUTPUT); // set power pin for DS18B20 to output
  digitalWrite(tempPower, HIGH); // turn sensor power on
  Sleepy::loseSomeTime(50); // Allow 50ms for the sensor to be ready
  // Start up the library
  sensors.begin();
  sensors.setWaitForConversion(false);
  numSensors = sensors.getDeviceCount();

  // search for one wire devices and copy to device address arrays.
  byte j = 0;
  while ((j < numSensors) && (oneWire.search(allAddress[j]))) {
    j++;
  }
}

void loop() {

  pinMode(tempPower, OUTPUT); // set power pin for DS18B20 to output
  digitalWrite(tempPower, HIGH); // turn DS18B20 sensor on
  Sleepy::loseSomeTime(50); // Allow 50 ms for the sensor to be ready
  sensors.requestTemperatures(); // Send the command to get temperatures


  for (int j = 0; j < numSensors; j++) {
    sensors.setResolution(allAddress[j], TEMPERATURE_PRECISION);      // and set the a to d conversion resolution of each.
  }

  sensors.requestTemperatures(); // Send the command to get temperatures
  Sleepy::loseSomeTime(ASYNC_DELAY); // Must wait for conversion, since we use ASYNC mode

  measurement.volt1[1] = (sensors.getTempC(allAddress[0]) * 100);
  if (numSensors > 1) {
    measurement.volt1[2] = (sensors.getTempC(allAddress[1]) * 100);
  }
  if (numSensors > 2) {
    measurement.volt1[3] = (sensors.getTempC(allAddress[2]) * 100);
  }
  if (numSensors > 3) {
    measurement.volt1[4] = (sensors.getTempC(allAddress[3]) * 100);
  }
  digitalWrite(tempPower, LOW); // turn DS18B20 sensor off
  pinMode(tempPower, INPUT); // set power pin for DS18B20 to input before sleeping, saves power

  // measure Vcc
  bitClear(PRR, PRADC); // power up the ADC
  ADCSRA |= bit(ADEN); // enable the ADC
  Sleepy::loseSomeTime(2);
  measurement.volt1[0] = readVcc(); // get supply voltage
  ADCSRA &= ~ bit(ADEN); // disable the ADC
  bitSet(PRR, PRADC); // power down the ADC

  PFETon();

  bitClear(PRR, PRUSI);    // enable USI h/w
  rf12_sleep(RF12_WAKEUP); // wake up RFM12B
  rf12_sendNow(0, &measurement, sizeof measurement);
  rf12_sendWait(2);
  rf12_sleep(RF12_SLEEP);  // put RFM12B to sleep
  bitSet(PRR, PRUSI);      // disable USI h/w

  PFEToff();

  Sleepy::loseSomeTime(MEASUREMENT_INTERVAL); //JeeLabs power save function: enter low power
}

//--------------------------------------------------------------------------------------------------
// Read current supply voltage
//--------------------------------------------------------------------------------------------------

long readVcc() {
  long result;
  // Read 1.1V reference against Vcc
  ADMUX = _BV(MUX5) | _BV(MUX0);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result; // Back-calculate Vcc in mV
  return result;
}
