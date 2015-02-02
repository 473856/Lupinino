//
// Receive RFM12B packets and log to
// 1) measurement data file = interpreted data, assuming from Temperature Node
//      --> time stamp, RF12 group ID, node ID, Vcc, T1, T2
// 2) raw data file
//      --> time stamp, RF12 group ID, node ID, all data bytes received
// 3) publish to MQTT server 192.168.1.50
//
// Blink LED #13 for each package received
//
// 150202 YunHub 1.03
//

#include <Bridge.h>
#include <JeeLib.h>
// #include "mytokens.h"

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

// data logging and time stamping from http://arduino.cc/en/Tutorial/YunDatalogger
#include <FileIO.h>
String fileName, dataStr, rawdataStr, emonapiurl;
char __fileName_rawdata[43] = "/mnt/sda1/datalogs/YYYY-MM-DD_RF12_raw.dat";  // value as pattern only, to get size right
char __fileName_data[39] = "/mnt/sda1/datalogs/YYYY-MM-DD_RF12.dat";  // value as pattern only, to get size right

#include <YunClient.h>
#include <PubSubClient.h>
char message_buff[255];
// Update these with values suitable for your network.
byte server[] = { 192, 168, 1, 50 };

// ===========================================================
// MQTT callback function, called for incoming messages
// ===========================================================
void callback(char *topic, byte *payload, unsigned int length)
{
    // counter
    int i = 0;
    // buffer for payload-to-string conversion
    char message_buff[100];

    Serial.println("Message arrived: topic: " + String(topic));
    Serial.println("Length: " + String(length, DEC));

    // copy payload to buffer, append \0
    for (i = 0; i < length; i++)
    {
        message_buff[i] = payload[i];
    }
    message_buff[i] = '\0';

    //convert to straing and print
    String msgString = String(message_buff);
    Serial.println("Payload: " + msgString);
}

YunClient yun;
PubSubClient client(server, 1883, callback, yun);

void setup ()
{
    // start-up the Yun bridge
    Bridge.begin();
    FileSystem.begin();
    delay(2000);

    Serial.begin(57600);
    Serial.println("***");
    Serial.println("*** 150202 YunHub 1.03");
    Serial.println("***");
    Serial.println("***  Log all incoming RF12 data to SD and publish to MQTT server 192.168.1.50");
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

    if (rf12_recvDone() && rf12_crc == 0)
    {

        digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)

        // Connect to MQTT server if not available
        if (!client.connected())
        {
            client.connect("arduinoYunClient");
            client.publish("ConnectMessage", "YunChen connected.");
        }

        // identify sender via group & nodeid
        // node id is in the first 5 bits of rf12_hdr --> & RF12_HDR_MASK
        dataPackage.rf12_group = rf12_grp;
        dataPackage.rf12_nodeid = rf12_hdr & RF12_HDR_MASK;


        //  Log and publish interpretated date for TNode (group 33, id 22)
        if (dataPackage.rf12_group == 33 and dataPackage.rf12_nodeid == 22)
        {
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
                Serial.print("Error opening data log file ");
                Serial.println(fileName);
            }

            // publish interpreted TNode data to MQTT server
            dataStr.toCharArray(message_buff, 255);
            if (client.publish("TNode", message_buff))
            {
                Serial.println("TNode data published to MQTT server.");
            }
        }

        //  Log and publish all raw data separately, to avoid loss or misinterpretation
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
            Serial.print("Error opening data log file ");
            Serial.println(fileName);
        }

        // publish to MQTT server
        rawdataStr.toCharArray(message_buff, 255);
        if (client.publish("RF12_RawData", message_buff))
        {
            Serial.println("Raw data published to MQTT server.");
        }

        Serial.println();

        //
    }
    digitalWrite(13, LOW);   // turn the LED off (HIGH is the voltage level)
    client.loop();
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

