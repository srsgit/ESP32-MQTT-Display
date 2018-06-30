/*
 Basic ESP8266 MQTT example

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

*/

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

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

const char* mqtt_server = "192.168.1.8";

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

  WiFiManager wifiManager;
  //reset saved settings
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect()) {
    ESP.restart();
    clearDisplay();
    delay(1000);
  }

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
