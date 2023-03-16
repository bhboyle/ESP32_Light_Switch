#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include <MQTT.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define NeoPixelPin 18
#define NUMPIXELS 1

// variables

int LightButton = 15;              // The input pin of the button that triggers the relay
int FactoryReset = 16;             // The input pin of the button that will trigger a factory reset
int RelayPin = 20;                 // The output pin that will trigger the relay
//                               0 ,    1      ,  2  ,         3   ,       4   ,         5   ,           6,      7
String variablesArray[8] = {"ssid", "password", "HostName", "MQTTIP", "UserName", "Password", "PublishTopic", "SubTopic"};
String valuesArray[8] = {"", "", "", "", "", "", "", ""};
int totalVariables = 8;
String index_html = "";
const char *ssidAP = "ESP32";
const char *passwordAP = "12345678";
IPAddress local_ipAP(192, 168, 1, 1);
IPAddress gatewayAP(192, 168, 1, 1);
IPAddress subnetAP(255, 255, 255, 0);
bool relayStatus = 0;
char ssid[32];
char ssidPassword[32];
char Hostname[32];
IPAddress MQTTserver; // address of the MQTT server
char MQTTuser[32];
char MQTTpass[32];
char PublishTopic[32];
char SubTopic[32];
bool allSet = false; // used for checking the web page
int inputError = -1;

// create objects
WiFiClient wifiClient; // create the wifi object
MQTTClient client;     // create the MQTT client object
Adafruit_NeoPixel pixels(NUMPIXELS, NeoPixelPin, NEO_GRB + NEO_KHZ800);
Preferences preferences;
AsyncWebServer server(80);

// function declarations
void checkWIFI();
void maintainMQTT();
void setColor(int r, int g, int b);
void getPrefs();
bool ValidateIP(String IP);
void createIndexHtml();
void handleSwitch();
void handle_NotFound(AsyncWebServerRequest *request);

// Start of setup function
void setup()
{

  getPrefs();

  pinMode(LightButton, INPUT_PULLUP);  // Setup the intput pin for the button that will trigger the relay
  pinMode(FactoryReset, INPUT_PULLUP); // setup the input pin for the button that will be used to reset the device to factory
  pinMode(RelayPin, OUTPUT);           // Setup the output pin for the relay
  // pinMode(WIFILEDPin, OUTPUT);         // Setup the output pin for the WIFI LED

  pixels.setBrightness(100); // Set BRIGHTNESS
  pixels.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();            // Set all pixel colors to 'off'

  setColor(255, 0, 0); // Set LED to red

  } // end of setup function

  // main loop function
  void loop()
  {

  checkWIFI();

  maintainMQTT();

  handleSwitch();

  } // End of main loop function

  // ****** Functions begin here

  // this function check is the switch button is pressed and toggles the light is it has
  void handleSwitch()
  {
  int temp = digitalRead(LightButton);
  if (temp == 0)
  {
    relayStatus = !relayStatus;
    digitalWrite(RelayPin, relayStatus);
  }
  }

  // MQTT callback function to handle any trigger events from the MQTT server
  void MQTTcallBack(String topic, String payload)
  {
  String tempChar = payload; // grab the payload
  if (tempChar == "0")       // check if the payload is a zero
  {
    relayStatus = 0;
    digitalWrite(RelayPin, relayStatus); // if it is a zero then turn off the light
    client.publish("Light/Status", "0");
  }
  else if (tempChar == "1") // if it is a one then turn on the light
  {
    relayStatus = 1;
    digitalWrite(RelayPin, relayStatus);
    client.publish("Light/Status", "1");
  }
  else if (tempChar == "2") // if it is a 2 then toggle the light
  {
    relayStatus = !relayStatus;
    digitalWrite(RelayPin, relayStatus);
    client.publish("Light/Status", String(relayStatus));
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
    WiFi.setHostname(Hostname); // Set the WIFI hostname of the device
    WiFi.begin(ssid, ssidPassword); // connect to the WIFI

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
    setColor(255, 255, 0); // Set LED to Yellow
  }
  } // end of checkwifi function

  // This function starts the MQTT connection and if it is already connected it will maintain the connection
  void maintainMQTT()
  {
    if (client.connected()) // used to maintain the MQTT connection
    {
      client.loop();
      setColor(0, 255, 0); // Set LED to Green
    }
    else // if the connection has failed then reconnect to the MQTT server
    {
      // connect to the MQTT server
      client.begin(MQTTserver, 1883, wifiClient);
      // connect(const char clientID[], const char username[], const char password[], bool skip = false);
      client.connect(Hostname, MQTTuser, MQTTpass, false);

      // publish/subscribe
      if (client.connected())
      {
        client.publish("hello/message", "hello world");
        client.subscribe(SubTopic);
        client.onMessage(MQTTcallBack);
        setColor(0, 255, 0); // Set LED to Green
      }
    }
  } // end of maintainMQTT function

  // This function sets the color of the NEOPixel LED. It must be called with
  // an R-ed intensity, an G-reen intensity and a B-lue intensity for the Neopixel
  void setColor(int r, int g, int b)
  {
    pixels.setPixelColor(0, pixels.Color(r, g, b));
    pixels.show();
  } // end of setColor function

  // This function will get the preferences from the ESP32 NV storage and if it has not yet been configured it will
  // out the ESP32 in AP mode to get it configured
  void getPrefs()
  {
    preferences.begin("configuration", false);
    bool configured = preferences.getBool("configured", false);
    if (configured)
    {
      for (int i = 0; i < totalVariables; i++)
      {
        valuesArray[i] = preferences.getString((variablesArray[i]).c_str(), "");
      }

      // put variable into usable variable in the right format
      int temp = valuesArray[0].length() + 1;
      valuesArray[0].toCharArray(ssid, temp);
      temp = valuesArray[1].length() + 1;
      valuesArray[1].toCharArray(ssidPassword, temp);
      temp = valuesArray[2].length() + 1;
      valuesArray[2].toCharArray(Hostname, temp);
      MQTTserver.fromString(valuesArray[3]);
      temp = valuesArray[4].length() + 1;
      valuesArray[4].toCharArray(MQTTuser, temp);
      temp = valuesArray[5].length() + 1;
      valuesArray[5].toCharArray(MQTTpass, temp);
      temp = valuesArray[6].length() + 1;
      valuesArray[6].toCharArray(PublishTopic, temp);
      temp = valuesArray[7].length() + 1;
      valuesArray[7].toCharArray(SubTopic, temp);

      checkWIFI();

      maintainMQTT();
    }
    else
    {
      WiFi.softAP(ssidAP, passwordAP);
      WiFi.softAPConfig(local_ipAP, gatewayAP, subnetAP);
      delay(100);

      createIndexHtml();

      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                { request->send_P(200, "text/html", index_html.c_str()); });

      server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
                {
      bool processedInput = false;
      for (int i = 0; i < totalVariables; i++) {
        if (request->hasParam(variablesArray[i])) {
          if (i == 3)
          {
            if (ValidateIP(request->getParam(variablesArray[i])->value())) {
              valuesArray[i] = request->getParam(variablesArray[i])->value();
              preferences.putString(variablesArray[i].c_str(), valuesArray[i]);
              processedInput = true;
            }
            else {  //show error message about the invalid input
              inputError = i;
              processedInput = true;
            }
          }
          else {
            valuesArray[i] = request->getParam(variablesArray[i])->value();
            preferences.putString(variablesArray[i].c_str(), valuesArray[i]);
            processedInput = true;
          }
        }
      }
      if (processedInput) {
        createAP_IndexHtml();
        request->send(200, "text/html", index_html.c_str());
      }
      if (allSet) {
        delay(1000);
        preferences.end();
        ESP.restart();
      } });
      server.onNotFound(handle_NotFound);
      server.begin();
    }
  } // end of getPrefs function

  void createAP_IndexHtml()
  {
    bool allEntered = true;
    index_html = "";
    String _type = "";
    index_html.concat("<!DOCTYPE HTML>");
    index_html.concat("<html>");
    index_html.concat("<head>");
    index_html.concat("<title>Smart Switch Configuration</title>");
    index_html.concat("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    index_html.concat("</head>");
    index_html.concat("<body>");
    index_html.concat("<h1> Welcome the the Bolye Smart Switch configuration page. Please note that all fields below are mandatory.</h1>");

    index_html.concat("<form action=\"/get\">");
    for (int i = 0; i < totalVariables; i++)
    {
      index_html.concat("<table>");
      index_html.concat("<tr height='15px'>");
      index_html.concat("<label style=\"display: inline-block; width: 141px;\">" + variablesArray[i] + ": </label>");
      if (i == 1 || i == 5)
      {
        _type = "password";
      }
      else
      {
        _type = "text";
      }
      if (inputError == i)
      {
        index_html.concat("<input style=\"background-color : red;\" type=\"" + _type + "\" name=\"" + variablesArray[i] + "\" value=\"" + valuesArray[i] + "\">");
        inputError = -1;
      }
      else
      {
        index_html.concat("<input type=\"" + _type + "\" name=\"" + variablesArray[i] + "\" value=\"" + valuesArray[i] + "\">");
      }
      // index_html.concat("<input type=\"submit\" value=\"Submit\">");
      index_html.concat("<br>");
      if (valuesArray[i] == "")
      {
        allEntered = false;
      }
      index_html.concat("</tr>");
      index_html.concat("</table>");
    }
    if (allEntered)
    {
      preferences.putBool("configured", true);
      allSet = true;
      index_html.concat("<H1>All fields are stored successfully. Please wait about 10 Seconds and then the device will be restarted.</H1><H1>If you need to change the settings you should reset the device.</H1>");
    }
    else
    {
      preferences.putBool("configured", false);
      allSet = false;
      index_html.concat("<input type=\"submit\" value=\"Submit\">");
    }
    index_html.concat("</form>");
    index_html.concat("</body>");
    index_html.concat("</html>");
  } // end of createIndexHtml function

  bool ValidateIP(String IP)
  {
    char *temp = (char *)IP.c_str();
    if (std::regex_match(temp, std::regex("^(?:(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\.){3}(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])$")))
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  void handle_NotFound(AsyncWebServerRequest *request)
  {
    request->send(404, "text/plain", "Not found");
  }