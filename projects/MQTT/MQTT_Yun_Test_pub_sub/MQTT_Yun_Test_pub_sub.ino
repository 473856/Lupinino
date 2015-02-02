/*
 Basic MQTT example

  - connects to an MQTT server
  - publishes "hello world" to the topic "outTopic"
  - subscribes to the topic "inTopic"
  - prints incoming messages to serial
*/

//#include <SPI.h>
#include <YunClient.h>
#include <PubSubClient.h>

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


void setup()
{
    Bridge.begin();
    delay(2000);

    if (client.connect("arduinoClient"))
    {
        client.publish("outTopic", "hello world");
        client.subscribe("inTopic");
    }
}

void loop()
{
    client.loop();
}

