#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include <MQTT.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <nvs_flash.h>
#include <ArduinoJson.h>
#include <AsyncElegantOTA.h>
#include "HTML.h"
#include <time.h>
#include "esp_sntp.h"
#include <CSE7766.h>
#include <regex>

#define NeoPixelPin 18
#define NUMPIXELS 1

#define MY_NTP_SERVER "pool.ntp.org"
#define MY_TZ "EST5EDT,M3.2.0,M11.1.0"
#define VersionControlFlag 17 // the pin to check to see if it is hardware version 2
#define Version1 20           // the pin the relay is on in hardware version 1
#define Version2 7            // the pin the relay is on in hardware version 2
#define BreathDelay 2         // how long to wait before starting the next stage int the LED breath
#define ResetTime 1000        // how many milliseconds to wait before doing factory reset
#define LightButton 15        // The input pin of the button that triggers the relay
#define FactoryReset 16       // The input pin of the button that will trigger a factory reset

// #define debug

// variables

int RelayPin;          // The output pin that will trigger the relay
//                               0 ,    1      ,  2  ,         3   ,       4   ,         5   ,           6,      7      ,      8,              9,...............
String variablesArray[10] = {"ssid", "password", "HostName", "MQTTIP", "UserName", "Password", "PublishTopic", "SubTopic", "RelayState", "LEDBrightness"};
String valuesArray[10] = {"", "", "", "", "", "", "", "", "", ""};
int totalVariables = 10;
String index_html_AP = "";            // String that will hold the web page code if the switch is not configured
String settingsHTML = "";             // String that will hold the settings web page code of a configured switch
const char *ssidAP = "BeBe-Light";    // Access point SSID if the switch is not configured
const char *passwordAP = "12345678";  // Access point password if the switch is not configured
IPAddress local_ipAP(192, 168, 1, 1); // Access point IP, Netmask and gateway settings  if not configured
IPAddress gatewayAP(192, 168, 1, 1);
IPAddress subnetAP(255, 255, 255, 0);
bool WifiAPStatus = false; // used to track if in access point mode or not and if not then connect to the configured network
bool relayStatus = 0;      // used to hold the status of the relay
char ssid[32];             // SSID variable for connecting to the configured network
char ssidPassword[32];     // SSID password for connecting to the configured network
char Hostname[32];         // The hostname of the device used for the DNS settings and the MQTT server
IPAddress MQTTserver; // address of the MQTT server
char MQTTuser[32];    // username for connecting to the MQTT server
char MQTTpass[32];    // Password for connecting to the MQTT server
char PublishTopic[32]; // The MQTT topic that the device will publish to with the status of the relay or other things.
char SubTopic[32];     // MQTT topic that we will subscribe to for triggering
bool allSet = false; // used for checking the web page
int inputError = -1; // used during processing if input fields on the configuration web page
unsigned long button_press_time = 0;   // used to debouncing the buttons
unsigned long button_release_time = 0; // also used for debouncing the buttons
const char *PARAM_INPUT = "value";     // used to handle the slider that sets the LED brightness on the settings webpage of configured switches
const char *PARAM_INPUT_1 = "state";   // used to manage the data that is sent to the ESP32 during configuration
bool MQTTinfoFlag = 0;                 // used because you can not send an MQTT message in the call back. This flag is turned on then we send a message in the loop
float Watts;                           // This is holding the Apparent power from the CSE7759
float testFrequency = 60;              // how often to check the current sensor  (Hz)
float Amps_TRMS;                           // estimated actual current in amps from the CSE7759
unsigned long currentReadInterval = 1000;  // how often to read the current sensor in milliseconds
unsigned long previousMillisSensor = 0;    // Used to track the last time we checked the current sensor
int lightButtonState = 0;                  // used for making sure the light button is only pressed once for each press and release
time_t now;                      // this is the epoch
tm tm;                           // the structure tm holds time information in a more convenient way
unsigned long lastTimeCheck = 0; // The last time we updated the time variables
bool OnState = 0;                // this is used to tell if there is current flowing through the switch or not. This is used to indicate on or off status
String sliderValue = "0";        // used to update the LED brightness value
bool LEDBreathDirection = 0;     // variable use for tracking if the LED is getting brighter or darker
unsigned long LEDLastTime;       // Used for keeping track of when the LED was last updated
int LEDBrightness = 0;           // This is the variable that holds the LED brightness value
bool configured = false;         // used to track if the switch is configured or not.

// This raw string is used to define the CSS styling for both versions of the configuration pages. Changes here will affect both pages.
String Style_HTML = R"---*(<style> 
input, textarea {border:3; padding:3px; border-radius: 8px 16px } 
input:focus { outline: none !important; border:3px solid OrangeRed; box-shadow: rgba(0, 0, 0, 0.3) 0px 8px 8px, rgba(0, 0, 0, 0.22) 0px 8px 8px; } 
</style>)---*";

// Object constructors
WiFiClient wifiClient; // create the wifi object
MQTTClient client;     // create the MQTT client object
Adafruit_NeoPixel pixels(NUMPIXELS, NeoPixelPin, NEO_GRB + NEO_KHZ800); // Create the object for the single Neopixel used for status indication
Preferences preferences;                                                // Create the object that will hold and get the variables from NVram
AsyncWebServer server_AP(80);                                           // Create the web server object
CSE7766 myCSE7759;

// function declarations

void checkWIFI();
void maintainMQTT();
void setColor(int r, int g, int b);
void getPrefs();
String convert2String(int temp);
bool ValidateIP(String IP);
void createAP_IndexHtml();
void handleSwitch();
void handle_NotFound(AsyncWebServerRequest *request);
void doSwitch(int status);
String outputState();
String processor(const String &var);
void checkFactoryReset();
void HandleMQTTinfo();
void checkCurrentSensor();
void updateTime();
void createSettingHTML();
void handleLEDBreath();

// Start of setup function
void setup()
{

  // check to see if the version control flag is set in hardware
  pinMode(VersionControlFlag, INPUT);
  delay(10); // I added this delay to make sure the input had settled before reading it status.
  if (digitalRead(VersionControlFlag))
  {
    RelayPin = Version2; // if it is set then set the relay pin to version 2
  }
  else
  {
    RelayPin = Version1; // if not then set it to version 1
  }

  pixels.setBrightness(100); // Set BRIGHTNESS of the indicator Neopixel
  pixels.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();            // Set all pixel colors to 'off'
  setColor(255, 0, 0);       // Set LED to red at boot up

  pinMode(LightButton, INPUT_PULLUP);  // Setup the intput pin for the button that will trigger the relay
  pinMode(FactoryReset, INPUT_PULLUP); // setup the input pin for the button that will be used to reset the device to factory
  pinMode(RelayPin, OUTPUT);           // Setup the output pin for the relay
  // pinMode(ACS_Pin, INPUT);             // Define the pin mode of the pin that reads the current sensor

#ifdef debug
  Serial.begin(115200);
#endif

  getPrefs();

  configTime(0, 0, MY_NTP_SERVER); // 0, 0 because we will use TZ in the next line
  setenv("TZ", MY_TZ, 1);          // Set environment variable with your time zone
  tzset();
  sntp_set_sync_interval(12 * 60 * 60 * 1000UL); // set the NTP server poll interval to every 12 hours

  LEDLastTime = millis();

  myCSE7759.setRX(10);
  myCSE7759.begin();

} // end of setup function

// main loop function
void loop()
{

  checkWIFI();

  maintainMQTT();

  handleSwitch();

  checkFactoryReset();

  HandleMQTTinfo();

  checkCurrentSensor();

  updateTime();

  handleLEDBreath();

} // End of main loop function

/*

****** Functions begin here   ***********

*/

// This function checks if the switch button is pressed and toggles the light is it has
void handleSwitch()
{
  int temp = digitalRead(LightButton); // read the button pin

  if (temp == 0) // if the button has been pressed
  {

    if (lightButtonState == 0) // if the button pressed variable is not set do the following
    {
      relayStatus = !relayStatus; // change the relay variable
      doSwitch(relayStatus);      // change the state of the relay
      lightButtonState = 1;       // set the button pressed variable
    }
  }
  else // if the button is not pressed reset the button pressed variable. This stops debounce and stops the button from re-triggering the relay if it is held pressed
  {
    lightButtonState = 0;
  }

} // end of handleSwitch function

// This function changes the relay output pin to the new state, publishes the new state
// to the MQTT server and stores the current state in NVram
void doSwitch(int status)
{
  digitalWrite(RelayPin, status);                 // set the relay to the sent status value
  client.publish(valuesArray[6], String(status)); // send the new switch state to the MQTT server in the Publish topic in the settings
  valuesArray[8] = status;                        // set the current relay state in the values array variable
  preferences.begin("configuration", false);
  preferences.putString(variablesArray[8].c_str(), valuesArray[8]); // store the relay state into NV ram
  preferences.end();

} // end of doSwitch function

// MQTT callback function to handle any trigger events from the MQTT server
void MQTTcallBack(String topic, String payload)
{
  String tempChar = payload; // grab the payload
  if (tempChar == "0")       // check if the payload is a zero
  {
    relayStatus = 0;
    doSwitch(relayStatus);
  }
  else if (tempChar == "off")
  {
    relayStatus = 0;
    doSwitch(relayStatus);
  }
  else if (tempChar == "false")
  {
    relayStatus = 0;
    doSwitch(relayStatus);
  }
  else if (tempChar == "1") // if it is a one then turn on the light
  {
    relayStatus = 1;
    doSwitch(relayStatus);
  }
  else if (tempChar == "on")
  {
    relayStatus = 1;
    doSwitch(relayStatus);
  }
  else if (tempChar == "true")
  {
    relayStatus = 1;
    doSwitch(relayStatus);
  }
  else if (tempChar == "2") // if it is a 2 then toggle the light
  {
    relayStatus = !relayStatus;
    doSwitch(relayStatus);
  }
  else if (tempChar == "info")
  {
    MQTTinfoFlag = true;
  }
} // end of MQTTcallback function

// This function will start the WIFI connection or maintains it if it is already connected
void checkWIFI()
{

  if (WifiAPStatus == 0)
  {
    if (WiFi.status() != WL_CONNECTED)
    {

#ifdef debug
      Serial.print("Hostname = ");
      Serial.println(Hostname);
#endif

      WiFi.setHostname(Hostname);     // Set the WIFI hostname of the device
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
  }

} // end of checkwifi function

// This function starts the MQTT connection and if it is already connected it will maintain the connection
void maintainMQTT()
{

  if (WifiAPStatus == 0)
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
// put the ESP32 in AP mode and start up a webpage to get it configured.
// It also has all the handlers for the web server in here like the /info handler that
// will generate a jason page that outputs the current status of the switch.
// There is a fair bit happening here.
void getPrefs()
{
  preferences.begin("configuration", false);
  configured = preferences.getBool("configured", false);
  if (configured)
  {

    WifiAPStatus = false;

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
    temp = valuesArray[8].length() + 1;
    char temp2[5];
    valuesArray[8].toCharArray(temp2, temp);
    relayStatus = atoi(temp2);
    doSwitch(relayStatus);
    temp = valuesArray[9].length() + 1;
    valuesArray[9].toCharArray(temp2, temp);
    pixels.setBrightness(atoi(temp2));

    checkWIFI();

    maintainMQTT();

    AsyncElegantOTA.begin(&server_AP); // Start AsyncElegantOTA

    // Route for root / web page   ****Start web server if the switch is configured
    server_AP.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send_P(200, "text/html", index_html, processor); });

    // handle the form data for the settings page
    server_AP.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
      bool processedInput = false;
      preferences.begin("configuration", false);
      for (int i = 0; i < totalVariables; i++)
          {
            if (request->hasParam(variablesArray[i]))
            {
                if (i==8){
                  valuesArray[8] = "0";
                } else {
                    valuesArray[i] = request->getParam(variablesArray[i])->value();
                }
                preferences.putString(variablesArray[i].c_str(), valuesArray[i]);
            }
          }
        preferences.end();
        delay(2000);
        ESP.restart(); }); // end of handling form data for settings page

    // Send a GET request to <ESP_IP>/refresh?state=<inputMessage>
    // used to change the switch state via http
    server_AP.on("/refresh", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
      String inputMessage;
      String inputParam;
      // GET input1 value on <ESP_IP>/refresh?state=<inputMessage>
      if (request->hasParam(PARAM_INPUT_1)) {
        inputMessage = request->getParam(PARAM_INPUT_1)->value();
        inputParam = PARAM_INPUT_1;
        relayStatus = !relayStatus;
        doSwitch(relayStatus);
      }
      else
      {
        inputMessage = "No message sent";
        inputParam = "none";
      }

      request->send(200, "text/plain", "OK"); });

    server_AP.on("/state", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send(200, "text/plain", String(relayStatus).c_str()); });

    // Send a GET request to <ESP_IP>/info
    // used to get the status of the switch state via HTTP
    // it will generate a JSON page with the current status
    server_AP.on("/info", HTTP_GET, [](AsyncWebServerRequest *request)
                 { 
                    
                    
                    String CurrentTime = convert2String(tm.tm_hour) + ":" + convert2String(tm.tm_min)  + ":" + convert2String(tm.tm_sec);
                    String CurrentDate = String(tm.tm_mon +1) + "/" + String(tm.tm_mday) + "/" + String(tm.tm_year +1900);
                    StaticJsonDocument<200> doc;
                    doc["Host Name"] = valuesArray[2];
                    doc["relayState"] = relayStatus;
                    doc["WiFiStrength"] = WiFi.RSSI();
                    doc["Date"] = CurrentDate;
                    doc["Time"] = CurrentTime;
                    if (digitalRead(VersionControlFlag)) // if it hardware version 2 or higher then display current
                    {
                      doc["Current"] = Amps_TRMS; // include how much current is currently flowing through the switch
                      doc["Watts"] = Watts;       // include how much current is currently flowing through the switch
                      doc["Voltage"] = myCSE7759.getVoltage();
                    }
                      char buffer[256];
                      serializeJson(doc, buffer);
                      request->send(200, "text/plain", buffer); });

    // generate the configuration file and send it for download
    server_AP.on("/export", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
      StaticJsonDocument<300> doc;
      doc["SSID"] = valuesArray[0];
      doc["network Password"] = valuesArray[1];
      doc["Host Name"] = valuesArray[2];
      doc["MQTT IP"] = valuesArray[3];
      doc["User Name"] = valuesArray[4];
      doc["MQTT Password"] = valuesArray[5];
      doc["Publish Topic"] = valuesArray[6];
      doc["Sub Topic"] = valuesArray[7];
      doc["Relay State"] = valuesArray[8];
      doc["LED Brightness"] = valuesArray[9];
      char buffer[300];
      serializeJson(doc, buffer);
      request->send(200, "text/plain", buffer); });

    createSettingHTML();

    server_AP.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send_P(200, "text/html", settingsHTML.c_str()); });

    // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
    server_AP.on("/slider", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
      String inputMessage;
      // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
      if (request->hasParam(PARAM_INPUT))
      {
        inputMessage = request->getParam(PARAM_INPUT)->value();
        sliderValue = inputMessage;
        pixels.setBrightness(sliderValue.toInt()); // Set BRIGHTNESS of the indicator Neopixel
        valuesArray[9] = sliderValue;
        preferences.begin("configuration", false);
        preferences.putString(variablesArray[9].c_str(), valuesArray[9]);
        preferences.end();
      }
      else
      {
        inputMessage = "No message sent";
      }
      request->send(200, "text/plain", "OK"); });

    // Start server
    server_AP.onNotFound(handle_NotFound);
    server_AP.begin();
  }
  else                   // if the board is not configured then do the following
  {                      // start the WIFI in AP mode and turn on the web server and display the config page
    setColor(0, 0, 255); // set the LED to blue to indicate the AP is active
    WiFi.softAP(ssidAP, passwordAP);
    WiFi.softAPConfig(local_ipAP, gatewayAP, subnetAP);
    WifiAPStatus = true;
    delay(100);

    createAP_IndexHtml();

    server_AP.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send_P(200, "text/html", index_html_AP.c_str()); });

    server_AP.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
    bool processedInput = false;
    for (int i = 0; i < totalVariables; i++)
    {
      if (request->hasParam(variablesArray[i]))
      {
        if (i == 3)
        {
          if (ValidateIP(request->getParam(variablesArray[i])->value()))
          {
            valuesArray[i] = request->getParam(variablesArray[i])->value();
            preferences.putString(variablesArray[i].c_str(), valuesArray[i]);
            processedInput = true;
          }
          else
          { // show error message about the invalid input
            inputError = i;
            processedInput = true;
          }
        }
        else if (i == 8)
        {
          valuesArray[i] = "0";
          preferences.putString(variablesArray[i].c_str(), valuesArray[i]);
        }
        else
        {
          valuesArray[i] = request->getParam(variablesArray[i])->value();
          preferences.putString(variablesArray[i].c_str(), valuesArray[i]);
          processedInput = true;
        }
      }
    }
    if (processedInput)
    {
      createAP_IndexHtml();
      request->send(200, "text/html", index_html_AP.c_str());
    }
    if (allSet)
    {
      
      delay(10000);
      ESP.restart();
    } });
    server_AP.onNotFound(handle_NotFound);
    server_AP.begin();
  }

} // end of getPrefs function

// This function is used by the /info web page to correctly display the time with
// leading zeros in front of numbers that are below 10
String convert2String(int temp)
{
  if (temp > 9)
  {
    return String(temp);
  }
  else
  {
    String result = "0";
    result += String(temp);
    return result;
  }
}

// This function generates the HTML for thw Access point mode at initial startup
void createAP_IndexHtml()
{
  bool allEntered = true;
  index_html_AP = "";
  String _type = "";
  index_html_AP.concat("<!DOCTYPE HTML>");
  index_html_AP.concat("<html>");
  index_html_AP.concat("<head>");
  index_html_AP.concat("<title>Smart Switch Configuration</title>");
  index_html_AP.concat("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  index_html_AP.concat(Style_HTML);
  index_html_AP.concat("</head>");
  index_html_AP.concat("<body>");
  index_html_AP.concat("<h2> Welcome to the BeBe Smart Switch configuration page. Please note that all fields below are mandatory.</h2>");
  index_html_AP.concat("<form action=\"/get\">");
  for (int i = 0; i < totalVariables; i++)
  {

    index_html_AP.concat("<table>");
    index_html_AP.concat("<tr height='15px'>");
    index_html_AP.concat("<label style=\"display: inline-block; width: 141px;\">" + variablesArray[i] + ": </label>");
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
      index_html_AP.concat("<input style=\"background-color : red;\" type=\"" + _type + "\" name=\"" + variablesArray[i] + "\" value=\"" + valuesArray[i] + "\">");
      inputError = -1;
    }
    else
    {
      index_html_AP.concat("<input type=\"" + _type + "\" name=\"" + variablesArray[i] + "\" value=\"" + valuesArray[i] + "\">");
    }
    // index_html.concat("<input type=\"submit\" value=\"Submit\">");
    index_html_AP.concat("<br>");
    if (valuesArray[i] == "")
    {
      allEntered = false;
    }
    index_html_AP.concat("</tr>");
    index_html_AP.concat("</table>");
  }

  if (allEntered)
  {
    preferences.putBool("configured", true);
    allSet = true;
    index_html_AP.concat("<H1>All fields are stored successfully. Please wait about 10 Seconds and then the device will be restarted.</H1><H1>If you need to change the settings you should reset the device.</H1>");
  }
  else
  {
    preferences.putBool("configured", false);
    allSet = false;
    index_html_AP.concat("<input type=\"submit\" value=\"Submit\">");
  }

  index_html_AP.concat("</form>");
  index_html_AP.concat("</body>");
  index_html_AP.concat("</html>");
} // end of createIndexHtml function

// This function makes sure that the IP addresses that are entered in the config page are valid IP addresses
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

} // end of ValidateIP function

// This function is an event handler for the wed server and it handles page requests for pages that do not exist
void handle_NotFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

// Replaces placeholder items in the main HTML with code generated as needed
String processor(const String &var)
{
  // Serial.println(var);
  if (var == "BUTTONPLACEHOLDER")
  {
    String buttons = "";
    String outputStateValue = outputState();
    buttons += "<h4>The " + valuesArray[2] + " light is <span id=\"outputState\"></span></h4><br><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  if (var == "SIGNALSTRENGTH")
  {
    String WIFI = "";
    int rssi = WiFi.RSSI();
    int strength;
    if (rssi <= -100)
    {
      strength = 0;
    }
    else if (rssi >= -50)
    {
      strength = 100;
    }
    else
    {
      strength = map((2 * (rssi + 100)), 0, 100, 0, 4);
    }

    WIFI.concat(String(rssi) + "<div class = 'waveStrength-" + String(strength) + "'>"); //    + String(strength) + "'>");
    WIFI.concat("<div class = 'wv4 wave' style = ''>");
    WIFI.concat("<div class = 'wv3 wave' style = ''>");
    WIFI.concat("<div class = 'wv2 wave' style = ''>");
    WIFI.concat("<div class = 'wv1 wave'>");
    WIFI.concat("</div></div></div></div></div> ");
    return WIFI;
  }
  return String();
} // end of processor function

// This function tells the web page what to display depending on the switch state.
String outputState()
{
  if (relayStatus)
  {
    return "checked";
  }
  else
  {
    return "";
  }
  return "";
} // end of outputState function

// This function checks if the Factory Reset button is pressed and
// if it is pressed for more than 10 seconds it will erase the
// preferences and reboot the ESP32
void checkFactoryReset()
{
  
    if (digitalRead(FactoryReset) == LOW) // see if the factory reset button has been pressed
    {

      if (button_press_time == 0) // if it has been pressed, start counting how long it has been pressed for
      {
        button_press_time = millis(); // take note of the time
      }
      if (millis() - button_press_time > ResetTime) // if it has been pressed for long enough, do the factory reset
      {
        for (int i = 0; i < 10; i++) // do light show to tell the user that a reset is imminent
        {
          setColor(255, 0, 0);
          delay(250);
          setColor(0, 0, 255);
          delay(250);
        }
        button_press_time = 0;
        nvs_flash_erase(); // reset the NV ram
        nvs_flash_init();
        ESP.restart(); // reboot ESP32
    }
  }

  // make sure that if the button is not pressed the button_press_time variable is reset
  if (digitalRead(FactoryReset))
  {
    button_press_time = 0;
  }

} // end of checkFactoryReset function

// This function check to see if there was a request to send device info and if so then it
// generates a json dataset and publishes it to the topic that was configured in teh settings
// also it resets the flag
void HandleMQTTinfo()
{

  if (MQTTinfoFlag) // check if the flag is set
  {
    // MQTTinfoFlag = false;                 // if it is, reset the flag
    // StaticJsonDocument<200> doc;          // start compiling the json dataset
    // doc["relayState"] = relayStatus;      // add the relay state
    // doc["WiFiStrength"] = WiFi.RSSI();    // add the WiFi signal strength
    // doc["Current"] = Amps_TRMS;           // include how much current is currently flowing through the switch
    // doc["Watts"] = Amps_TRMS * 127;       // include how much current is currently flowing through the switch

    // char buffer[256];                     // start the conversion to a Char
    // serializeJson(doc, buffer);           // do the conversion
    // client.publish(PublishTopic, buffer); // send the message

    MQTTinfoFlag = false;
    String temp = "0";

    if (relayStatus)
    {
        temp = "1";
    }
    else
    {
        temp = "0";
    }

    client.publish(PublishTopic, temp); // send the message
  }

} // end of HandleMQTTinfo function

// The function reads the current sensor and reports the data over time
void checkCurrentSensor()
{

  if ((millis() - previousMillisSensor) > currentReadInterval) // check to see if enough time has passed before checking the Current sensor
  {
    previousMillisSensor = millis(); // update time
    myCSE7759.handle();
    // ACS_Value = myCSE7759.getCurrent(); // ACS_Value currently not used
    Amps_TRMS = myCSE7759.getCurrent();   // Check the CSE7759 and get its current
    Watts = myCSE7759.getApparentPower(); // get watts
  }

  // ******************************************************
  // the following is only going to work on Gen 3 boards
  // uncomment this for newer boards and adjust the reporting in the other functions
  // to use this data to determine if the switch is on or off

  // if (Amps_TRMS > .02) { // if the current is above a basic value then assume the switch is on
  //   OnState = true;
  // } else {  // if not then assume the switch is off
  //   OnState = false;
  //  }
  //  ****************************************************

} // end of checkCurrenSensor function

// This function makes sure the device always knows the current time
// and keeps that time in the TM struct
// ***This will be used later to track power use over time***
void updateTime()
{

  if ((millis() - lastTimeCheck) > currentReadInterval)
  {
    lastTimeCheck = millis();
    time(&now);             // read the current time
    localtime_r(&now, &tm); // update the structure tm with the current time
  }

  // Serial.print(tm.tm_year + 1900);    // years since 1900
  // Serial.print(tm.tm_mon + 1);        // January = 0 (!)
  // Serial.print(tm.tm_mday);           // day of month
  // Serial.print(tm.tm_hour);           // hours since midnight 0-23
  // Serial.print(tm.tm_min);            // minutes after the hour 0-59
  // Serial.print(tm.tm_sec);            // seconds after the minute 0-61*
  // Serial.print(tm.tm_wday);           // days since Sunday 0-6
  // if (tm.tm_isdst == 1)               // Daylight Saving Time flag

} // end of updateTimeStamp Function

// This function generates the HTML page for the settings page for the
// configured switch.
void createSettingHTML()
{
  // get the current IP address
  IPAddress temp = WiFi.localIP();
  String IP = String(temp[0]) + "." + String(temp[1]) + "." + String(temp[2]) + "." + String(temp[3]); // and convert it to a string for use in a link
  settingsHTML = "";
  settingsHTML.concat("<!DOCTYPE HTML>");
  settingsHTML.concat("<html>");
  settingsHTML.concat("<head>");
  settingsHTML.concat("<title>Smart Switch Configuration</title>");
  settingsHTML.concat("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  settingsHTML.concat(Style_HTML);
  settingsHTML.concat("</head>");
  settingsHTML.concat("<script>");
  settingsHTML.concat("function updateSliderPWM(element)");
  settingsHTML.concat("{");
  settingsHTML.concat(" var sliderValue = document.getElementById('brightnessSlider').value;");
  settingsHTML.concat("console.log(sliderValue);");
  settingsHTML.concat("var xhr = new XMLHttpRequest();");
  settingsHTML.concat("xhr.open('GET', '/slider?value=' + sliderValue, true);");
  settingsHTML.concat("xhr.send();");
  settingsHTML.concat("}");
  settingsHTML.concat("</script>");
  settingsHTML.concat("<body>");
  settingsHTML.concat("<h2> Welcome to the BeBe Smart Switch Settings page. </h2><br>");
  settingsHTML.concat("<form action=\"/get\" onsubmit='setTimeout(function() { window.location.reload(); }, 7000)'>");
  settingsHTML.concat("Wifi SSID <input type='text' name=\"" + variablesArray[0] + "\" value=\"" + valuesArray[0] + "\">");
  settingsHTML.concat("<br><br>");
  settingsHTML.concat("Wifi Password <input type='password' name=\"" + variablesArray[1] + "\" value=\"" + valuesArray[1] + "\">");
  settingsHTML.concat("<br> <br>");
  settingsHTML.concat("Device Hostname <input type='text' name=\"" + variablesArray[2] + "\" value=\"" + valuesArray[2] + "\">");
  settingsHTML.concat("<br><br>");
  settingsHTML.concat("MQTT Server Host IP <input type='text' name=\"" + variablesArray[3] + "\" value=\"" + valuesArray[3] + "\">");
  settingsHTML.concat("<br><br>");
  settingsHTML.concat("MQTT Username <input type='text' name=\"" + variablesArray[4] + "\" value=\"" + valuesArray[4] + "\">");
  settingsHTML.concat("<br><br>");
  settingsHTML.concat("MQTT Password <input type='password' name=\"" + variablesArray[5] + "\" value=\"" + valuesArray[5] + "\">");
  settingsHTML.concat("<br><br>");
  settingsHTML.concat("Publish Topic <input type='text' name=\"" + variablesArray[6] + "\" value=\"" + valuesArray[6] + "\">");
  settingsHTML.concat("<br><br>");
  settingsHTML.concat("Subscribe Topic <input type='text' name=\"" + variablesArray[7] + "\" value=\"" + valuesArray[7] + "\">");
  settingsHTML.concat("<br><br>");
  settingsHTML.concat("LED Brightness <input type='range' onchange = 'updateSliderPWM(this)' id='brightnessSlider' min='10' max='255' value=" + valuesArray[9] + " step = '1' class='slider' id='myRange'>");
  settingsHTML.concat("<br><br>");
  settingsHTML.concat("<input type='submit' value='Submit'> <input type='button' value='Cancel' onclick='location.href=\"http:\/\/" + IP + "\"'/>");
  settingsHTML.concat("</form>");
  settingsHTML.concat("<br>Once the form is submitted the page will reload after 7 seconds");
  settingsHTML.concat("<div align='right'><a href='http:\/\/" + IP + "/export' download='" + valuesArray[2] + ".cfg' >Download Config</a> </div>");
  settingsHTML.concat("</body>");
  settingsHTML.concat("</html>");

} // end of createSettingHTML function

// This function makes the LED "breath" depending on the relay state
// If the relay is off then the LED will breath to indicate the relay is off.
// If the relay is on the LED will state solid to indicate the relay is on.
void handleLEDBreath()
{

  // Check to see if the switch has been configured. If it has then breath the LED.
  // If this is not done the switch will go into a reboot cycle if the switch is not
  // configured due to the accessing the valuesArray[9] while the switch is not configured
  if (configured)
  {
    if ((millis() - LEDLastTime) > (BreathDelay + (255 / valuesArray[9].toInt() * 4))) // only adjust the display intensity if the correct amount of time has elapsed
    {

        if (relayStatus == false) // if the relay is off then breath the LED
        {
          if (LEDBreathDirection == true) // check to see whether or not to count up or down
          {
          LEDBrightness++;                            // count up
          if (LEDBrightness > valuesArray[9].toInt()) // if at the top of the scale
          {
            LEDBreathDirection = false; // change directions
          }
          pixels.setBrightness(LEDBrightness); // set the LED to the new intensity
          }
          else // if we need to count down
          {
          LEDBrightness--;       // count down
          if (LEDBrightness < 1) // dont go below zero
          {
            LEDBreathDirection = true; // reset count direction if at zero
          }
          pixels.setBrightness(LEDBrightness); // set the LED to the new intensity
          }
        }
        else // if the Relay is on
        {
          pixels.setBrightness(valuesArray[9].toInt()); // set the LED to the maximum set point
        }
        LEDLastTime = millis(); // keep track of the time for the next cycle
    }
  }

} // end of handle LEDbreath function