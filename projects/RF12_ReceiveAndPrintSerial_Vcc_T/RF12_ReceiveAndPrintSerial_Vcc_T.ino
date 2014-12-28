//
// Receive packets from Temperate Node. Print RF12 group ID, node ID, Vcc, T.
//
// 141227 473856@posteo.org
//

#include <JeeLib.h>

typedef struct
{
    float Vcc;  // Supply voltage
    float T;    // Temperature reading
} measurementStruc;

measurementStruc measurement;


void setup ()
{
    Serial.begin(57600);
    Serial.println(57600);
    Serial.println("Receive packets from Temperate Node, print RF 12 group ID, node ID, Vcc, T");

    rf12_initialize(1, RF12_868MHZ, 0); // group id = 0 to receive from all groups
}

void loop ()
{
    if (rf12_recvDone() && rf12_crc == 0)
    {

        //        examine packet byte by byte
        //        for (byte i = 0; i < rf12_len; ++i) {
        //            Serial.print(rf12_data[i]);
        //            Serial.print(' ');
        //            }

        // assuming payload structure is correct
        measurement.Vcc = (rf12_data[0] + 256 * rf12_data[1]) / 100.0;
        measurement.T = (rf12_data[2] + 256 * rf12_data[3]) / 100.0;
        Serial.print("rf12 group = "); Serial.print(rf12_grp);

        // node id is in the first 5 bits of rf12_hdr --> & RF12_HDR_MASK
        Serial.print(", rf12 node id  = "); Serial.print(rf12_hdr & RF12_HDR_MASK);

        Serial.print(", Vcc = "); Serial.print(measurement.Vcc);
        Serial.print(", T = "); Serial.println(measurement.T);
    }
}
