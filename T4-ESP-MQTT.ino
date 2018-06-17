/*
 Basic ESP8266 MQTT example

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

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

const char* ssid        = "ssid";
const char* password    = "password";
const char* mqtt_server = "mqtt.server.local";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;


void drawBackground() {
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextWrap(false);

}


void setup() {

  Serial.begin(115200);
  drawBackground();

  setup_wifi();

  tft.setTextSize(1);
  tft.setCursor(200, 310);
  tft.println("MQTT v1");
 
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {

  delay(2000);
  // We start by connecting to a WiFi network

  tft.setTextSize(1);
  tft.setCursor(0,310);
  tft.print("Connecting to ");
  tft.println(ssid);
  
  WiFi.enableSTA(true);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(1500);
    Serial.print(".");
  }

  tft.fillRect(0,310,240,10,ILI9341_BLACK);
  tft.setCursor(0,310);
  tft.print("IP address: ");
  tft.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String topicStr = (char *)topic;

  if (topicStr == "time/seconds") {
    callbackTime(topic, payload, length);
  } else if (topicStr == "shed.temp") {
    callbackShedTemp(topic, payload, length);
  } else if (topicStr == "bed.temp") {
    callbackBedTemp(topic, payload, length);
  }
}

String oldTime = "                     ";
void callbackTime(char* topic, byte* payload, unsigned int length) {

  String nowDate = (char *) payload;
  nowDate = nowDate.substring(0,10);
  String nowTime = (char *) payload;
  nowTime = nowTime.substring(11,16);

  if (nowTime != oldTime) {
    tft.setFont(&FreeMono18pt7b);
    tft.setTextColor(ILI9341_BLUE);
    //tft.setTextSize(3);
    tft.fillRect(15,10,210,30,ILI9341_BLACK);
    tft.setCursor(15,30);
    tft.print(nowDate);
    tft.setFont();

    tft.setFont(&FreeMonoBold24pt7b);
    tft.setTextColor(ILI9341_RED);
    //tft.setTextSize(5);
    tft.fillRect(50,40,195,50,ILI9341_BLACK);
    tft.setCursor(50,80);
    tft.print(nowTime);
    tft.setFont();
    
    oldTime = nowTime;
  }
}

String oldShedTemp = "00.00";
void callbackShedTemp(char* topic, byte* payload, unsigned int length) {

  String nowTemp = (char *) payload;
  nowTemp = nowTemp.substring(0,length);

  if (nowTemp != oldShedTemp) {
    tft.setFont(&FreeMono18pt7b);
    tft.setTextColor(ILI9341_GREEN);
    //tft.setTextSize(1);
    tft.fillRect(6,150,230,35,ILI9341_BLACK);
    tft.setCursor(6,180);
    tft.print("Shed: " + nowTemp);
    oldShedTemp = nowTemp;
    tft.setFont();
  }
}

String oldBedTemp = "00.00";
void callbackBedTemp(char* topic, byte* payload, unsigned int length) {

  String nowTemp = (char *) payload;
  nowTemp = nowTemp.substring(0,length);

  if (nowTemp != oldBedTemp) {
    tft.setFont(&FreeMono18pt7b);
    tft.setTextColor(ILI9341_GREEN);
    //tft.setTextSize(1);
    tft.fillRect(6,186,230,35,ILI9341_BLACK);
    tft.setCursor(6,210);
    tft.print("Bed:  " + nowTemp);
    oldBedTemp = nowTemp;
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
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("time/seconds");
      client.subscribe("shed.temp");
      client.subscribe("bed.temp");
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
