//
// Receive RFM12B packets and log to
// 1) measurement data file = interpreted data, assuming from Temperature Node
//      --> time stamp, RF12 group ID, node ID, Vcc, T1, T2
// 2) raw data file
//      --> time stamp, RF12 group ID, node ID, all data bytes received
// 3) If from JNµ Temperature Node, send every EMONCMS_SEND_INTERVAL ms via http to emoncms.org
//
// Blink LED #13 for each package received
//
// 150110 YunHub 1.02
//

#include <Bridge.h>
#include <HttpClient.h>
#include <JeeLib.h>
#include "mytokens.h"

// temperature node data structure. Timestamp created locally, at time of receival.
typedef struct
{
    String timeStamp;   // "+%Y-%m-%d %T": format "YYYY-MM-DD hh:mm:ss"
    byte rf12_group;    // RF12 group ID
    byte rf12_nodeid;   // RF12 node ID
    float Vcc;          // Supply voltage
    float T1;            // Temperature reading
    float T2;            // Temperature reading
} dataPackageStruc;

dataPackageStruc dataPackage;

// send every EMONCMS_SEND_INTERVAL milliseconds
#define EMONCMS_SEND_INTERVAL 60000
unsigned long timeLastSent = 0;

// data logging and time stamping from http://arduino.cc/en/Tutorial/YunDatalogger
#include <FileIO.h>
String fileName, dataStr, rawdataStr, emonapiurl;
char __fileName_rawdata[43] = "/mnt/sda1/datalogs/YYYY-MM-DD_RF12_raw.dat";  // value as pattern only, to get size right
char __fileName_data[39] = "/mnt/sda1/datalogs/YYYY-MM-DD_RF12.dat";  // value as pattern only, to get size right

void setup ()
{
    // start-up the Yun bridge
    Bridge.begin();
    FileSystem.begin();
    delay(2000);

    Serial.begin(57600);
    Serial.println("***");
    Serial.println("*** 150111 YunHub 1.02");
    Serial.println("***");
    Serial.println("***  Log all incoming RF12 data. Send JNµ temperature data to emoncms.org.");
    Serial.println("***");
    Serial.println("*** Format:     time stamp, RF12 group ID, node ID, Vcc, T1, T2");
    Serial.println("***             time stamp, RF12 group ID, node ID, raw data bytes");
    Serial.println("***");
    Serial.println("*** Logging to  /mnt/sda1/datalogs/YYYY-MM-DD_RF12.dat");
    Serial.println("***             /mnt/sda1/datalogs/YYYY-MM-DD_RF12_raw.dat");
    Serial.println("***");
    
    // initialize digital pin 13 as an output.
    pinMode(13, OUTPUT);

    rf12_initialize(1, RF12_868MHZ, 0); // group id = 0 to receive from all groups
}


void loop ()
{

    HttpClient client;

    if (rf12_recvDone() && rf12_crc == 0)
    {

        /*
            // debugging only: examine packet byte by byte
            for (byte i = 0; i < rf12_len; ++i) {
              Serial.print(rf12_data[i]);
              Serial.print(' ');
            }
        */

        digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)

        // assuming payload structure is correct
        dataPackage.rf12_group = rf12_grp;
        dataPackage.rf12_nodeid = rf12_hdr & RF12_HDR_MASK; // node id is in the first 5 bits of rf12_hdr --> & RF12_HDR_MASK
        dataPackage.Vcc = int(word(rf12_data[1], rf12_data[0])) / 1000.0;
        dataPackage.T1 = int(word(rf12_data[3], rf12_data[2])) / 100.0;
        dataPackage.T2 = int(word(rf12_data[5], rf12_data[4])) / 100.0;

        //
        // Log measurement data to file
        //
        dataPackage.timeStamp = getTimeStamp();
        dataStr = dataPackage.timeStamp + ","
                  + String(dataPackage.rf12_group) + ","
                  + String(dataPackage.rf12_nodeid) + ","
                  + String(dataPackage.Vcc) + ","
                  + String(dataPackage.T1) + ","
                  + String(dataPackage.T2);

        // open file, one one can be open at a time
        fileName = "/mnt/sda1/datalogs/"
                   + dataPackage.timeStamp.substring(0, 10)
                   + "_RF12.dat";
        // convert fileName string to char array
        fileName.toCharArray(__fileName_data, sizeof(__fileName_data));
        File dataFile = FileSystem.open(__fileName_data, FILE_APPEND);

        // if the file is available, write to it:
        if (dataFile)
        {
            dataFile.println(dataStr);
            dataFile.close();
            // print to the serial port too:
            Serial.println(dataStr);
        }
        // if the file isn't open, pop up an error:
        else
        {
            Serial.println("error opening measurement data log file");
        }

        // Log raw data to separate file, to prevent data gets lost or misinterpreted
        //
        rawdataStr = dataPackage.timeStamp + ","
                     + String(dataPackage.rf12_group) + ","
                     + String(dataPackage.rf12_nodeid);
        for (byte i = 0; i < rf12_len; ++i)
        {
            rawdataStr += ",";
            rawdataStr += rf12_data[i];
        }
        // open file, one one can be open at a time
        fileName = "/mnt/sda1/datalogs/"
                   + dataPackage.timeStamp.substring(0, 10)
                   + "_RF12_raw.dat";
        // convert fileName string to char array
        fileName.toCharArray(__fileName_rawdata, sizeof(__fileName_rawdata));
        File rawdataFile = FileSystem.open(__fileName_rawdata, FILE_APPEND);

        // if the file is available, write to it:
        if (rawdataFile)
        {
            rawdataFile.println(rawdataStr);
            rawdataFile.close();
            // print to the serial port too:
            Serial.println(rawdataStr);
        }
        // if the file isn't open, pop up an error:
        else
        {
            Serial.println("error opening raw data log file");
        }

        // if from JeeNode µ Temperature Node, then send data for to emoncms.org: node id, Vcc, T1, T2
        if ((timeLastSent == 0 or millis() - timeLastSent > EMONCMS_SEND_INTERVAL)
                and dataPackage.rf12_group == 33 and dataPackage.rf12_nodeid == 22)
        {
            emonapiurl = "http://emoncms.org/input/post.json?apikey="
                         + EMONCMS_APIKEY
                         + "&node="
                         + String(dataPackage.rf12_nodeid)
                         + "&csv="
                         + String(dataPackage.Vcc) + ","
                         + String(dataPackage.T1) + ","
                         + String(dataPackage.T2);

            client.get(emonapiurl);
            timeLastSent = millis();
            Serial.println(emonapiurl);
        }
        Serial.println();

        //
    }
    digitalWrite(13, LOW);   // turn the LED off (HIGH is the voltage level)
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// This function returns a string with the time stamp
String getTimeStamp()
{
    String result;
    Process time;
    // date is a command line utility to get the date and the time
    // in different formats depending on the additional parameter
    time.begin("date");
    time.addParameter("+%Y-%m-%d %T");  // parameters: D for the complete date mm/dd/yy, T for the time hh:mm:ss
    time.run();  // run the command

    // read the output of the command
    while (time.available() > 0)
    {
        char c = time.read();
        if (c != '\n')
            result += c;
    }

    return result;
}

