#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <tr064.h>
#include <PubSubClient.h>

//Die SSID Ihres WLANs
const char* wifi_ssid = "WLANNAME(SSID)";
//Das Passwort Ihres WLANs
const char* wifi_password = "WLANPASSWORT";

//Der Benutzername für Ihre Fritzbox. Wenn Sie keine separaten Benutzer angelegt haben, lautet dieser "admin".
const char* fuser = "admin";

//Das Passwort für Ihre Fritzbox
const char* fpass = "PASSWORT";

//Die IP-Adresse Ihrer Fritzbox. Ab Werk lautet diese 192.168.178.1.
const char* IP = "192.168.178.1";

const int PORT = 49000;
TR064 connection(PORT, IP, fuser, fpass);

//Damit der ESP nicht unnötig Zeit mit der DHCP-Aushandlung verbringt und sich das Klingelsignal verzögert, vergeben Sie eine statische
//Adresse aus Ihrem Subnetz.
//In den Werkseinstellungen der Fritz!Box lautet diese 192.168.178.1.
IPAddress staticIP(192, 168, 178, 230);
//Hier erneut die Fritzbox-IP durch Kommata separiert eintragen
IPAddress gateway(192,168,178,1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(gateway);

//Wenn Sie einen MQTT-Server haben, setzen Sie den Parameter auf 1 und
#define mqttenabled 0
//tragen die IP-Adresse des Servers in den nächsten Parameter ein.
IPAddress mqttserver(192, 168, 7, 109);

#if mqttenabled == 1
WiFiClient espClient;
PubSubClient client(espClient);
#endif
 
const char* deviceName = "ESP-Klingel";

void setup() {
  #if mqttenabled == 1
  client.setServer(mqttserver, 1883);
  #endif
  WiFi.hostname(deviceName);
  WiFi.config(staticIP, subnet, gateway, dns);
  WiFi.begin(wifi_ssid, wifi_password);
  WiFi.mode(WIFI_STA);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
  }
 
  connection.init();
  #if mqttenabled == 1
  client.connect(deviceName);
  #endif

  //Der zweite Parameter ist die Rufnummer, die gewählt wird. **9 ist der Fritzbox-Rundruf.
  String params[][2] = {{"NewX_AVM-DE_PhoneNumber", "**9"}};
 
  String req[][2] = {{}};
  String params1[][2] = {{}};
  connection.action("urn:dslforum-org:service:X_VoIP:1","X_AVM-DE_DialNumber", params, 1, req, 0);

 
  #if mqttenabled ==1
  client.publish("esp-klingel/klingel", "Ring Ring");
  #endif
 
  //Hier können Sie die Zeit, die das Telefon klingelt, in Millisekunden einstellen
  delay(4000);
 
  connection.action("urn:dslforum-org:service:X_VoIP:1","X_AVM-DE_DialHangup", params1, 1, req, 0);
 
  ESP.deepSleep(0);
}

void loop(){

}



