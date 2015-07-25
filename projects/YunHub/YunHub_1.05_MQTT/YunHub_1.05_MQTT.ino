//
// Receive RFM12B packets and publish to  MQTT server 192.168.178.27
// 1) Temperature Node (group 33, node 22)
//      --> time stamp, RF12 group ID, node ID, Vcc, T1, T2
// 2) RFM12B raw data
//      --> time stamp, RF12 group ID, node ID, all data bytes received
//
// Blink LED #13 for each package received
//
// 150725 YunHub 1.05
//

#include <Bridge.h>
#include <JeeLib.h>
#include <YunClient.h>
#include <PubSubClient.h>

byte rf12_group, rf12_nodeid;
String timeStamp, dataStr, rawdataStr;
char message_buff[255];

// Update these with values suitable for your network.
byte server[] = { 192, 168, 178, 27 };


// Temperature node data structure. Timestamp created locally, at time of receival.
struct
{
    String timeStamp;   // "+%Y-%m-%d %T": format "YYYY-MM-DD hh:mm:ss"
    byte rf12_group;    // RF12 group ID
    byte rf12_nodeid;   // RF12 node ID
    float Vcc;          // Supply voltage
    float T1;            // Temperature reading
    float T2;            // Temperature reading
} TNodeData;


// Temperature node data structure. Timestamp created locally, at time of receival.
struct
{
    String timeStamp;   // "+%Y-%m-%d %T": format "YYYY-MM-DD hh:mm:ss"
    byte rf12_group;    // RF12 group ID
    byte rf12_nodeid;   // RF12 node ID
    float Vcc;          // Supply voltage
} RadioBlip2Data;


// ===========================================================
// MQTT callback function, called for incoming messages
// ===========================================================
void callback(char *topic, byte *payload, unsigned int length)
{
    // counter
    int i = 0;
    // buffer for payload-to-string conversion
    char message_buff[255];

    // copy payload to buffer, append \0
    for (i = 0; i < length; i++)
    {
        message_buff[i] = payload[i];
    }
    message_buff[i] = '\0';
}

YunClient yun;
PubSubClient client(server, 1883, callback, yun);

void setup ()
{
    // start-up the Yun bridge
    Bridge.begin();
    delay(2000);

    Serial.begin(57600);
    Serial.println("***");
    Serial.println("*** 150725 YunHub 1.05");
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
        rf12_group = rf12_grp;
        rf12_nodeid = rf12_hdr & RF12_HDR_MASK;
        timeStamp = getTimeStamp();


        ////////////////////////////////////
        //  Raw Data (group ALL, id ALL)  //
        ////////////////////////////////////
        rawdataStr = timeStamp + ","
                     + String(rf12_group) + ","
                     + String(rf12_nodeid);
        for (byte i = 0; i < rf12_len; ++i)
        {
            rawdataStr += ",";
            rawdataStr += rf12_data[i];
        }

        // publish to MQTT server
        rawdataStr.toCharArray(message_buff, 255);
        client.publish("RF12_RawData", message_buff);


        ///////////////////////////////
        //  TNode (group 33, id 22)  //
        ///////////////////////////////
        if (rf12_group == 33 and rf12_nodeid == 22)
        {
            TNodeData.rf12_group = rf12_group;
            TNodeData.rf12_nodeid = rf12_nodeid;
            TNodeData.timeStamp = timeStamp;
            TNodeData.Vcc = int(word(rf12_data[1], rf12_data[0])) / 1000.0;
            TNodeData.T1 = int(word(rf12_data[3], rf12_data[2])) / 100.0;
            TNodeData.T2 = int(word(rf12_data[5], rf12_data[4])) / 100.0;
 
           dataStr = TNodeData.timeStamp + ","
                      + String(TNodeData.rf12_group) + ","
                      + String(TNodeData.rf12_nodeid) + ","
                      + String(TNodeData.Vcc) + ","
                      + String(TNodeData.T1) + ","
                      + String(TNodeData.T2);

            // publish interpreted TNode data to MQTT server
            dataStr.toCharArray(message_buff, 255);
            client.publish("TNode", message_buff);
        }

        ////////////////////////////////////
        //  RadioBlip2 (group 33, id 17)  //
        ////////////////////////////////////
        if (rf12_group == 33 and rf12_nodeid == 17)
        {
            RadioBlip2Data.rf12_group = rf12_group;
            RadioBlip2Data.rf12_nodeid = rf12_nodeid;
            RadioBlip2Data.timeStamp = timeStamp;
            RadioBlip2Data.Vcc = 1.0 + rf12_data[5] * 0.02;
 
           dataStr = RadioBlip2Data.timeStamp + ","
                      + String(RadioBlip2Data.rf12_group) + ","
                      + String(RadioBlip2Data.rf12_nodeid) + ","
                      + String(RadioBlip2Data.Vcc);
                      
            // publish interpreted TNode data to MQTT server
            dataStr.toCharArray(message_buff, 255);
            client.publish("RadioBlip2", message_buff);
        }

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

