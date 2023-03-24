#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include <MQTT.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <nvs_flash.h>
#include <ArduinoJson.h>

#define NeoPixelPin 18
#define NUMPIXELS 1
// #define debug

// variables

int LightButton = 15;  // The input pin of the button that triggers the relay
int FactoryReset = 16; // The input pin of the button that will trigger a factory reset
int RelayPin = 20;     // The output pin that will trigger the relay
//                               0 ,    1      ,  2  ,         3   ,       4   ,         5   ,           6,      7      ,      8,              9
String variablesArray[10] = {"ssid", "password", "HostName", "MQTTIP", "UserName", "Password", "PublishTopic", "SubTopic", "RelayState", "LEDBrightness"};
String valuesArray[10] = {"", "", "", "", "", "", "", "", "", ""};
int totalVariables = 10;
String index_html_AP = "";            // String that will hold the web page code if the switch is not configured
const char *ssidAP = "Boyle-Light";   // Access point SSID if the switch is not configured
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
const char *PARAM_INPUT_1 = "state";   // used to manage the data that is sent to the ESP32 during configuration
bool MQTTinfoFlag = 0;                 // used because you can not send an MQTT message in the call back. This flag is turned on then we send a message in the loop

// The following raw literal is holding the web page that is used when the switch is configured. This web page displays
// a visual representation of the switch that lets you turn the light on and off. It also updates the on screen switch
// if you push the button
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Boyle Smart Switch</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 1.5rem;}
    p {font-size: 3.0rem;}
    div#bottom-right {position: absolute; bottom: -55px; right: 0px}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px; transform: rotate(-90deg)} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    .wave {
        display: inline-block;
        border: 10px solid transparent;
        border-top-color: currentColor;
        border-radius: 50%%;
        border-style: solid;
        margin: 5px;
    }
    .waveStrength-3 .wv4.wave,
    .waveStrength-2 .wv4.wave,
    .waveStrength-2 .wv3.wave,
    .waveStrength-1 .wv4.wave,
    .waveStrength-1 .wv3.wave,
    .waveStrength-1 .wv2.wave {
        border-top-color: #eee;
    }
  </style>
</head>
<body>
  <h2>Boyle smart light switch</h2>
  <div id=bottom-right>%SIGNALSTRENGTH%</div>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); }
  else { xhr.open("GET", "/update?state=0", true); }
  xhr.send();
}

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var inputChecked;
      var outputStateM;
      if( this.responseText == 1){ 
        inputChecked = true;
        outputStateM = "On";
      }
      else { 
        inputChecked = false;
        outputStateM = "Off";
      }
      document.getElementById("output").checked = inputChecked;
      document.getElementById("outputState").innerHTML = outputStateM;
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}, 1000 ) ;
</script>
</body>
</html>
)rawliteral";

// Object constructors
WiFiClient wifiClient; // create the wifi object
MQTTClient client;     // create the MQTT client object
Adafruit_NeoPixel pixels(NUMPIXELS, NeoPixelPin, NEO_GRB + NEO_KHZ800);
Preferences preferences;
AsyncWebServer server_AP(80);

// function declarations

void checkWIFI();
void maintainMQTT();
void setColor(int r, int g, int b);
void getPrefs();
bool ValidateIP(String IP);
void createAP_IndexHtml();
void handleSwitch();
void handle_NotFound(AsyncWebServerRequest *request);
void doSwitch(int status);
String outputState();
String processor(const String &var);
void checkFactoryReset();
void HandleMQTTinfo();

// HTTPSRedirect *client = nullptr;

// Start of setup function
void setup()
{
#ifdef debug
  Serial.begin(115200);
#endif

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

  checkFactoryReset();

  HandleMQTTinfo();

} // End of main loop function

/*

****** Functions begin here   ***********

*/

// This function check is the switch button is pressed and toggles the light is it has
void handleSwitch()
{
  int temp = digitalRead(LightButton);
  if (temp == 0)
  {
    relayStatus = !relayStatus;
    doSwitch(relayStatus);
  }
} // end of handleSwitch function

// This function changes the relay output pin to the new state, publishes the new state
// to the MQTT server and stores the current state in NVram
void doSwitch(int status)
{
  digitalWrite(RelayPin, status);
  client.publish("Light/Status", String(status));
  valuesArray[8] = status;
  preferences.begin("configuration", false);
  preferences.putString(variablesArray[8].c_str(), valuesArray[8]);
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
  else if (tempChar == "1") // if it is a one then turn on the light
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

  // This function will start the WIFI connection or maintain it if it is already connected
  void checkWIFI()
  {

  if (!WifiAPStatus)
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
  if (!WifiAPStatus)
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
  // put the ESP32 in AP mode and start up a webpage to get it configured
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
    temp = valuesArray[8].length() + 1;
    char temp2[5];
    valuesArray[8].toCharArray(temp2, temp);
    relayStatus = atoi(temp2);
    doSwitch(relayStatus);
    valuesArray[9].toCharArray(temp2, temp);
    pixels.setBrightness(atoi(temp2));

    checkWIFI();

    maintainMQTT();

    // Route for root / web page   ****Start web server if the switch is configured
    server_AP.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send_P(200, "text/html", index_html, processor); });

    // Send a GET request to <ESP_IP>/update?state=<inputMessage>
    // used to change the switch state via http
    server_AP.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
      String inputMessage;
      String inputParam;
      // GET input1 value on <ESP_IP>/update?state=<inputMessage>
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

    // Send a GET request to <ESP_IP>/state
    // used to get the status of the switch state via HTTP
    server_AP.on("/info", HTTP_GET, [](AsyncWebServerRequest *request)
                 { 
                    StaticJsonDocument<200> doc;
                    doc["relayState"] = relayStatus;
                    doc["WiFiStrength"] = WiFi.RSSI();
                    char buffer[256];
                    serializeJson(doc, buffer);
                    request->send(200, "text/plain", buffer); });
    // Start server
    server_AP.begin();
    }
    else // if the board is not configured then do the following
    {
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
      preferences.end();
      delay(10000);
      ESP.restart();
    } });
    server_AP.onNotFound(handle_NotFound);
    server_AP.begin();
    // handleSwitch();
    }
  } // end of getPrefs function

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
    index_html_AP.concat("</head>");
    index_html_AP.concat("<body>");
    index_html_AP.concat("<h2> Welcome the the Bolye Smart Switch configuration page. Please note that all fields below are mandatory.</h2>");

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
    // ********* This needs to be fixed. It was crashing the ESP32 when a configuration was sent.*********
    // char *temp = (char *)IP.c_str();
    // std::regex ipv4("(([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\.){3}([0- 9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])");
    // if (std::regex_match(temp, ipv4))
    return true;
    // else
    // return false;
  } // end of ValidateIP function

  // This function is an event handler for the wed server and it handles page requests for pages that do not exist
  void handle_NotFound(AsyncWebServerRequest *request)
  {
    request->send(404, "text/plain", "Not found");
  }

  // Replaces placeholder with button section in your web page
  String processor(const String &var)
  {
    // Serial.println(var);
    if (var == "BUTTONPLACEHOLDER")
    {
      String buttons = "";
      String outputStateValue = outputState();
      buttons += "<h4>The light is <span id=\"outputState\"></span></h4><br><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label>";
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
    if (digitalRead(FactoryReset) == LOW)
    {
      if (button_press_time == 0)
      {
          button_press_time = millis();
          Serial.println("button pressed.");
      }
      if (millis() - button_press_time > 10000)
      {
          for (int i = 0; i < 10; i++)
          {
        setColor(255, 0, 0);
        delay(250);
        setColor(0, 0, 0);
        delay(250);
          }
          button_press_time = 0;
          nvs_flash_erase();
          nvs_flash_init();
          ESP.restart();
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
      MQTTinfoFlag = false;                 // if it is reset the flag
      StaticJsonDocument<200> doc;          // start compiling the json dataset
      doc["relayState"] = relayStatus;      // add the relay state
      doc["WiFiStrength"] = WiFi.RSSI();    // add the WiFi signal strength
      char buffer[256];                     // start the conversion to a Char
      serializeJson(doc, buffer);           // do the conversion
      client.publish(PublishTopic, buffer); // send the message
    }
  } // end of HandleMQTTinfo function