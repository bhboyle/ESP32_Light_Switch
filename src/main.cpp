#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include <MQTT.h>

#define NeoPixelPin 18
#define NUMPIXELS 1

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
int RelayPin = 20;                 // The output pin that will trigger the relay
//                               0 ,    1      ,  2  ,      3   ,     4   ,         5   ,       6       ,       7        ,          8      ,          9       ,       10    ,           11
String variablesArray[12] = {"ssid", "password", "IP", "Gateway", "subnet", "PrimaryDNS", "SecondaryDNS", "GScriptIdRead", "GScriptIdWrite", "MeetingRoomName", "googleHost", "googleHttpsPort"};
String valuesArray[12] = {"", "", "", "", "", "", "", "", "", "", "", ""};
int totalVariables = 12;

// create objects
WiFiClient wifiClient; // create the wifi object
MQTTClient client;     // create the MQTT client object
Adafruit_NeoPixel pixels(NUMPIXELS, NeoPixelPin, NEO_GRB + NEO_KHZ800);
Preferences preferences;

// contructors
void checkWIFI();
void maintainMQTT();
void setColor(int r, int g, int b);

// Start of setup function
void setup()
{

  pinMode(LightButton, INPUT_PULLUP);  // Setup the intput pin for the button that will trigger the relay
  pinMode(FactoryReset, INPUT_PULLUP); // setup the input pin for the button that will be used to reset the device to factory
  pinMode(RelayPin, OUTPUT);           // Setup the output pin for the relay
  // pinMode(WIFILEDPin, OUTPUT);         // Setup the output pin for the WIFI LED

  pixels.setBrightness(100); // Set BRIGHTNESS
  pixels.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();            // Set all pixel colors to 'off'

  // analogWrite(WIFILEDPin, 0); // Set the WIFI Led to off at startup

  setColor(0, 255, 0);

  checkWIFI();

  maintainMQTT();

  } // end of setup function

  // main loop function
  void loop()
  {

    checkWIFI();

    maintainMQTT();

  } // End of main loop function

  // ****** Functions begin here

  // MQTT callback function to handle any trigger events from the MQTT server
  void MQTTcallBack(String topic, String payload)
  {
    String tempChar = payload; // grab the payload
    if (tempChar == "0")       // check if the payload is a zero
    {
      digitalWrite(RelayPin, LOW); // if it is a zero then turn off the light
      client.publish("Light/Status", "0");
    }
    else if (tempChar == "1") // if it is a one then turn on the light
    {
      digitalWrite(RelayPin, HIGH);
      client.publish("Light/Status", "1");
    }
    else if (tempChar == "2") // if it is a 2 then toggle the light
    {
      int temp = !digitalRead(RelayPin);
      digitalWrite(RelayPin, temp);
      client.publish("Light/Status", String(temp));
    }
  } // end of MQTTcallback function

  // This function will start the WIFI connection or maintain it if it is already connected
  void checkWIFI()
  {

    if (WiFi.status() != WL_CONNECTED)
    {

      // Configures static IP address. We use a static IP because it connects to the network faster

      // WiFi.reconnect();
      WiFi.mode(WIFI_STA);        // set the WIFI mode to Station
      WiFi.setHostname(HOSTNAME); // Set the WIFI hostname of the device
      WiFi.begin(ssid, password); // connect to the WIFI

      delay(100);

      int count = 0; // create a variable that is used to set a timer for connecting to the WIFI
      while (WiFi.status() != WL_CONNECTED)
      {             // check if it is connected to the AP
        delay(100); // wait 100 miliseconds
        count++;    // Increment the counter variable
        if (count > 30)
        {
          break;
        } // it is had been trying to connect to the WIFI for 30 * 100 milliseconds then stop trying
      }

      // This second WIFI status is used to do indicate a failed connection and then put the ESP32 in deep sleep
      if (WiFi.status() != WL_CONNECTED)
      { // if it can not connect to the wifi
        // write the boot verion and count into eeprom and reboot

        ESP.restart();
      }
    }
  } // end of checkwifi function

  // This function starts the MQTT connection and if it is already connected it will maintain the connection
  void maintainMQTT()
  {
    if (client.connected()) // used to maintain the MQTT connection
    {
      client.loop();
    }
    else // if the connection has failed then reconnect to the MQTT server
    {
      // connect to the MQTT server
      client.begin(server, 1883, wifiClient);
      // connect(const char clientID[], const char username[], const char password[], bool skip = false);
      client.connect(HOSTNAME, USERNAME, PASSWORD, false);

      // publish/subscribe
      if (client.connected())
      {
        client.publish("hello/message", "hello world");
        client.subscribe("Lights/Kitchen");
        client.onMessage(MQTTcallBack);
      }
    }
  } // end of maintainMQTT function

  // this function sets the color of the NEOPixel led. it must be called with
  // an R intensity, an G intensity and a B intensity
  void setColor(int r, int g, int b)
  {
    pixels.setPixelColor(0, pixels.Color(r, g, b));
    pixels.show();
  } // end of setColor function
