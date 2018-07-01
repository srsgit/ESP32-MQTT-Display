/*
 Basic ESP8266 MQTT example

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

*/
// flag to format file syste and lose any saved config
#define DEBUG_CLEAN

#include <FS.h>
#include <FSImpl.h>
#include <vfs_api.h>
#include "SPIFFS.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMono18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#define TFT_CS   27
#define TFT_DC   26
#define TFT_MOSI 23
#define TFT_CLK  18
#define TFT_RST  5
#define TFT_MISO 12
//#define TFT_LED   5  // GPIO not managed by library

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

// Update these with values suitable for your network.

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

//define your default values here, if there are different values in config.json, they are overwritten.
//length should be max size + 1 
char mqtt_server[40] = "";
char mqtt_port[6] = "1883";
//default custom static IP
char static_ip[16] = "192.168.1.20";
char static_gw[16] = "192.168.1.254";
char static_sn[16] = "255.255.255.0";

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void clearDisplay() {
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextWrap(false);

  tft.setFont();
  tft.setTextSize(1);
  tft.setCursor(260, 230);
  tft.println("MQTT v2");
}

void drawBackground() {
  clearDisplay();
  tft.setFont(&FreeMono12pt7b);
  tft.setTextColor(ILI9341_GREEN);
  //tft.fillRect(6,150,230,35,ILI9341_BLACK);
  tft.setCursor(6,90);
  tft.print("Shed");

  tft.setCursor(113,90);
  tft.print("Study");

  tft.setCursor(216,90);
  tft.print("Bedroom");

  tft.drawRoundRect(  0,5,  159,55,3, ILI9341_WHITE);
  tft.drawRoundRect(161,5,  159,55,3, ILI9341_WHITE);
  tft.drawRoundRect(  0,70, 105,60,3, ILI9341_WHITE);
  tft.drawRoundRect(106,70, 105,60,3, ILI9341_WHITE);
  tft.drawRoundRect(212,70, 108,60,3, ILI9341_WHITE);
  
  tft.setFont();
}

void configModeCallback (WiFiManager *myWiFiManager) {
  clearDisplay();
  
  tft.setFont(&FreeMono12pt7b);
  tft.setTextColor(ILI9341_RED);
  tft.setCursor(40,20);
  tft.println("WiFi Config Mode");

  tft.setFont(&FreeMono9pt7b);
  tft.setCursor(40,110);
  tft.println("Connect to network:\n");
  //if you used auto generated SSID, print it

  tft.setFont(&FreeMono12pt7b);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(50,140);
  tft.println(myWiFiManager->getConfigPortalSSID());

  tft.setFont(&FreeMono9pt7b);
  tft.setTextColor(ILI9341_RED);
  tft.setCursor(40,165);
  tft.println("to setup device WiFi");
}

void setup() {

  Serial.begin(115200);
  tft.begin();
  clearDisplay();
  WiFi.mode(WIFI_STA);

#ifdef DEBUG_CLEAN
  //clean FS, for testing
  //SPIFFS.format();
#endif

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);

          if(json["ip"]) {
            Serial.println("setting custom ip from config");
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
            Serial.println(static_ip);

          } else {
            Serial.println("no custom ip in config");
          }
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);

  WiFiManager wifiManager;
  //reset saved settings
  //wifiManager.resetSettings();

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.setTimeout(180);

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);

  delay(3000);
  if (!wifiManager.autoConnect()) {
    clearDisplay();
    ESP.restart();
    delay(5000);
  }

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;

    json["ip"]      = WiFi.localIP().toString();
    json["gateway"] = WiFi.gatewayIP().toString();
    json["subnet"]  = WiFi.subnetMask().toString();

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("\nlocal ip");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());
  Serial.println("MQTT Server");
  Serial.println(mqtt_server);

  drawBackground();

  tft.setFont();
  tft.setTextSize(1);
  tft.fillRect(0,230,260,10,ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(12,230);
  tft.print("IP address: ");
  tft.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String topicStr = (char *)topic;

  if (topicStr == "time/seconds") {
    callbackTime(topic, payload, length);
  } else if (topicStr == "shed.temp") {
    callbackShedTemp(topic, payload, length);
  } else if (topicStr == "bed.temp") {
    callbackBedTemp(topic, payload, length);
  } else if (topicStr == "study.temp") {
    callbackStudyTemp(topic, payload, length);
  }

}

String oldTime = "                     ";
void callbackTime(char* topic, byte* payload, unsigned int length) {

  String nowDate = (char *) payload;
  nowDate = nowDate.substring(0,10);
  String nowTime = (char *) payload;
  nowTime = nowTime.substring(11,16);

  if (nowTime != oldTime) {
    tft.setFont(&FreeMonoBold24pt7b);
    tft.setTextColor(ILI9341_RED);
    tft.fillRect(1,6,156,53,ILI9341_BLACK);
    tft.setCursor(10,44);
    tft.print(nowTime);
    tft.setFont();
    
    tft.setFont(&FreeMono12pt7b);
    tft.setTextColor(ILI9341_BLUE);
    tft.fillRect(162,6,156,53,ILI9341_BLACK);
    tft.setCursor(170,44);
    tft.print(nowDate);
    tft.setFont();
    
    oldTime = nowTime;
  }
}

String oldShedTemp = "00.00";
void callbackShedTemp(char* topic, byte* payload, unsigned int length) {

  String nowTemp = (char *) payload;
  nowTemp = nowTemp.substring(0,length);

  if (nowTemp != oldShedTemp) {
    tft.setFont(&FreeMono12pt7b);
    tft.setTextColor(ILI9341_GREEN);
    tft.fillRect(6,96,94,28,ILI9341_BLACK);
    tft.setCursor(6,118);
    tft.print(nowTemp);
    oldShedTemp = nowTemp;
    tft.setFont();
  }
}

String oldBedTemp = "00.00";
void callbackBedTemp(char* topic, byte* payload, unsigned int length) {

  String nowTemp = (char *) payload;
  nowTemp = nowTemp.substring(0,length);

  if (nowTemp != oldBedTemp) {
    tft.setFont(&FreeMono12pt7b);
    tft.setTextColor(ILI9341_GREEN);
    tft.fillRect(214,96,94,28,ILI9341_BLACK);
    tft.setCursor(216,118);
    tft.print(nowTemp);
    oldBedTemp = nowTemp;
    tft.setFont();
  }
}

String oldStudyTemp = "00.00";
void callbackStudyTemp(char* topic, byte* payload, unsigned int length) {

  String nowTemp = (char *) payload;
  nowTemp = nowTemp.substring(0,length);

  if (nowTemp != oldStudyTemp) {
    tft.setFont(&FreeMono12pt7b);
    tft.setTextColor(ILI9341_GREEN);
    tft.fillRect(113,96,94,28,ILI9341_BLACK);
    tft.setCursor(113,118);
    tft.print(nowTemp);
    oldStudyTemp = nowTemp;
    tft.setFont();
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("time/seconds");
      client.subscribe("shed.temp");
      client.subscribe("bed.temp");
      client.subscribe("study.temp");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
