#include <WiFi.h>
#include <MQTT.h>

#define WIFILEDPin 18

// ***** Change the following section for each device.
#define HOSTNAME "KitchenLight" // the wifi and MQTT host name. this should be changed for each device
#define USERNAME "bboyle"
#define PASSWORD "Hanafranastan1!"
#define ssid "gallifrey"
#define password "rockstar"

WiFiClient wifiClient;
IPAddress server(172, 17, 17, 10); // address of the MQTT server
MQTTClient client;                 //(server, 1883, MQTTcallBack); // create the MQTT client object

void MQTTcallBack(String topic, String payload)
{
    String tempChar = payload;
    if (tempChar == "0")
    {
        digitalWrite(WIFILEDPin, LOW);
        client.publish("Light/Status", "0");
    }
    else if (tempChar == "1")
    {
        digitalWrite(WIFILEDPin, HIGH);
        client.publish("Light/Status", "1");
    }
    else if (tempChar == "2")
    {
        int temp = !digitalRead(WIFILEDPin);
        digitalWrite(WIFILEDPin, temp);
        client.publish("Light/Status", String(temp));
    }
}

void setup()
{

    pinMode(WIFILEDPin, OUTPUT);

    WiFi.setHostname(HOSTNAME);
    WiFi.begin(ssid, password);

    // connect to the MQTT server
    client.begin(server, 1883, wifiClient);
    // connect(const char clientID[], const char username[], const char password[], bool skip = false);
    client.connect(HOSTNAME, USERNAME, PASSWORD, false);

    // publish/subscribe
    if (client.connected())
    {
        client.publish("hello/message", "hello world");
        // client.subscribe("door/control");
        client.subscribe("Lights/Kitchen");
        client.onMessage(MQTTcallBack);
    }

    client.subscribe("Lights/Kitchen");
}

void loop()
{
}