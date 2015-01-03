//
// Receive packets from Temperate Node. Print RF12 group ID, node ID, Vcc, T.
//

#include <Bridge.h>
#include <JeeLib.h>

typedef struct
{
    byte rf12_group;
    byte rf12_nodeid;
    float Vcc;  // Supply voltage
    float T;    // Temperature reading
} dataPackageStruc;

dataPackageStruc dataPackage;


void setup ()
{
    // start-up the Yun bridge
    Bridge.begin();
    delay(2000);

    Serial.begin(57600);
    Serial.println("Receive packets from Temperature Node, print RF 12 group ID, node ID, Vcc, T");

    rf12_initialize(1, RF12_868MHZ, 0); // group id = 0 to receive from all groups
}

void loop ()
{
    if (rf12_recvDone() && rf12_crc == 0)
    {
        /*
            // debugging only: examine packet byte by byte
            for (byte i = 0; i < rf12_len; ++i) {
              Serial.print(rf12_data[i]);
              Serial.print(' ');
            }
        */

        // assuming payload structure is correct
        dataPackage.rf12_group = rf12_grp;
        dataPackage.rf12_nodeid = rf12_hdr & RF12_HDR_MASK; // node id is in the first 5 bits of rf12_hdr --> & RF12_HDR_MASK
        dataPackage.Vcc = word(rf12_data[1], rf12_data[0]) / 1000.0;
        dataPackage.T = word(rf12_data[3], rf12_data[2]) / 100.0;

        Serial.print("rf12 group = "); Serial.print(dataPackage.rf12_group);
        Serial.print(", rf12 node id = "); Serial.print(dataPackage.rf12_nodeid);
        Serial.print(", Vcc = "); Serial.print(dataPackage.Vcc);
        Serial.print(", T = "); Serial.println(dataPackage.T);
    }
}
