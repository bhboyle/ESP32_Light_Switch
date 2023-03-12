#include <WiFi.h>
#include <MQTT.h>

#define WIFILEDPin 18

// ***** Change the following section for each device.
#define HOSTNAME "KitchenLight" // the wifi and MQTT host name. this should be changed for each device
#define USERNAME "bboyle"
#define PASSWORD "Hanafranastan1!"
#define ssid "gallifrey"
#define password "rockstar"

// variables
IPAddress server(172, 17, 17, 10); // address of the MQTT server
int LightButton = 15;              // The input pin of the button that triggers the relay
int FactoryReset = 16;             // The input pin of the button that will trigger a factory reset
int RelayPin = 20;                 // The output pin that will trigger the realy

WiFiClient wifiClient; // create the wifi object
MQTTClient client;     // create the MQTT client object

// setup function
void setup()
{

    pinMode(LightButton, INPUT_PULLUP);  // Setup the intput pin for the button that will trigger the relay
    pinMode(FactoryReset, INPUT_PULLUP); // setup the input pin for the button that will be used to reset the device to factory
    PinMode(RelayPin, OUTPUT);           // Setup the output pin for the relay
    pinMode(WIFILEDPin, OUTPUT);         // Setup the output pin for the WIFI LED

    analogWrite(WIFILEDPin, 0); // Set the WIFI Led to off at startup

    WiFi.setHostname(HOSTNAME); // Set the WIFI hostname of the device
    WiFi.begin(ssid, password); // connect to the WIFI

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
} // end of setup function

// main loook function
void loop()
{

} // End od main loop function

// ****** Functions begin here

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
