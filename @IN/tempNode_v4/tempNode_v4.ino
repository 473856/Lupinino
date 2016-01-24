// tempNode V4
// Simple Temperature Node, using a single DS18B20
// assumes powered by AA power board 
// see http://jeelabs.net/boards/7/topics/3898
// Damon Bound 2012-08-14

#include <JeeLib.h>
#include <avr/sleep.h>
#include <OneWire.h>

#define DEBUG   0               // set to 1 to trace activity via serial console

#define SET_NODE        4       // wireless node ID 
#define SET_GROUP       100     // wireless net group 
#define SEND_MODE       2       // set to 3 if fuses are e=06/h=DE/l=CE, else set to 2
#define ONEWIRE_PIN     6       // on arduino digital pin 6 = jeenode port 3 DIO line
                                // on arduino digital pin 7 = jeenode port 4 DIO line
#define BATT_SENSE_PORT 2       // sense battery voltage on this port

#define PREPTEMP_DELAY  8       // how long to wait for DS18B20 sensor to stabilise, in tenths of seconds
#define MEASURE_INTERVAL  592   // how long between measurements, in tenths of seconds
                                // 592 + 8 = 600 i.e. 1 minute
#if DEBUG
static const char signature[14] = "[tempNodeV4]";
#endif
                                
// define OneWire object for our DS18B20 temperature sensor
OneWire ds(ONEWIRE_PIN);  
// define JeeLib Port for the battery reading
Port battPort (BATT_SENSE_PORT);

// The JeeLib scheduler makes it easy to perform various tasks at various times:
enum { PREPTEMP, MEASUREALL, TASK_END };
static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// This defines the structure of the packets which get sent out by wireless:
struct {
    uint32_t packetseq;     // packet serial number 32 bits 1..4294967295
    int tempDS1820B;        // temperature * 16 as 2 bytes
    byte battVolts;         // battery voltage V * 100 as 1 byte - assumes AA Power Board 
                            // i.e. V is nominally 1.50 and always less than 2.55 so one byte is enough
} payload;

// because we're using the watchdog timer for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// send out a radio packet with our data payload
static byte sendPayload () {
  ++payload.packetseq;

  rf12_sleep(RF12_WAKEUP);
  while (!rf12_canSend())
    rf12_recvDone();
  rf12_sendStart(0, &payload, sizeof payload);
  rf12_sendWait(SEND_MODE);
  rf12_sleep(RF12_SLEEP);
}

// prepare to read out temperature sensor
static void prepTemp() {

    // starts a temperature measurement cycle
    // then there needs to be a delay for DS18B20 to do its thing 
    // at least 750ms @ 12-bit resolution, we actually wait 8 tenths = 800 ms
    // the DS18B20 automatically goes into low power standby after its conversion is done
    byte i;
    byte present = 0;
    byte type_s;
    byte data[12];
    byte addr[8];

    if ( !ds.reset() ) return;          // reset before performing any comms with onewire device
    ds.reset_search();                  // start new search
    ds.search(addr);                    // get first device
    ds.reset();                         // restart comms
    ds.select(addr);                    // select this device
    ds.write(0x44);                     // issue command: start conversion
      
}

static int readTemp() {
    // read the temperature back from the DS18B20 that we kicked off a little while ago
    byte i;
    byte present = 0;
    byte type_s;
    byte data[12];
    byte addr[8];

    if ( !ds.reset() ) return 0;        // reset before performing any comms with onewire device
    ds.reset_search();                  // start new search
    ds.search(addr);                    // get first device
    ds.reset();                         // restart comms
    ds.select(addr);                    // select this device
    ds.write(0xBE);                     // issue command: read scratchpad
    for ( i = 0; i < 9; i++) {          // we need 9 bytes
        data[i] = ds.read();
            }
    ds.depower();
    int raw = (data[1] << 8) | data[0];
    unsigned char t_mask[4] = {0x7, 0x3, 0x1, 0x0};
    byte cfg = (data[4] & 0x60) >> 5;
    raw &= ~t_mask[cfg];
    return raw;
}

static byte readBatt() {
    byte count = 4;
    int value;
    while (count-- > 0) {
        value = battPort.anaRead();
    }
    return  (byte) map(value,0,1013,0,330);
}

// readout all the sensors and other values
static void doMeasure() {
    
  payload.tempDS1820B = readTemp();
  payload.battVolts = readBatt();
  sendPayload();

    #if SERIAL
        dumpPayload("doMeasure()");
    #endif
}

static void serialFlush () {
    #if ARDUINO >= 100
        Serial.flush();
        delay(2); // make sure tx buf is empty before going back to sleep
    #endif  
}

void setup() {
    rf12_initialize(SET_NODE, RF12_915MHZ, SET_GROUP);
    rf12_sleep(RF12_SLEEP);
    
    #if DEBUG
        Serial.begin(57600);
        Serial.print(signature);
        serialFlush();
    #endif
    
    scheduler.timer(PREPTEMP, 0);       // start the measurement loop going

}

void loop() {

    // call to scheduler.pollWaiting() waits in sleep mode until
    // the next event is due
    switch (scheduler.pollWaiting()) {

        case PREPTEMP:
           #if DEBUG
               Serial.println("action=prep temp");
               serialFlush();
           #endif

           prepTemp();

           // schedule next action after the appropriate delay
           scheduler.timer(MEASUREALL, PREPTEMP_DELAY);

           break;

        case MEASUREALL:
            #if DEBUG
                Serial.println("action=measureall");
                serialFlush();
            #endif
    
            doMeasure();

            // schedule next measurement periodically
            scheduler.timer(PREPTEMP, MEASURE_INTERVAL);

            break;
        
    }   
}

