//
// Receive packets from JNu Temperature node
// Print Vcc, Temperature

#include <JeeLib.h>

float Vcc, T;

void setup () {
    Serial.begin(57600);
    Serial.println(57600);
    Serial.println("Receive");
    rf12_initialize(1, RF12_868MHZ, 33);
}

void loop () {
    if (rf12_recvDone() && rf12_crc == 0) {
        for (byte i = 0; i < rf12_len; ++i) {
            Serial.print(rf12_data[i]);
            Serial.print(' ');
            }    
        Serial.println();
        
        Vcc = (rf12_data[0] + 256 * rf12_data[1])/100.0;
        T = (rf12_data[2] + 256 * rf12_data[3])/100.0;        
        Serial.println(Vcc);
        Serial.println(T);        
        Serial.println();
    }
    
        delay(4000); // otherwise led blinking isn't visible
}
