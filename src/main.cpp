#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#include "DHT.h"

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

#define ssid "INEA-B16F"
#define pass "5554874968"
#define host "korest.herokuapp.com"
#define port 80
#define USE_SERIAL Serial
#define MESSAGE_INTERVAL 2000
#define DHTPIN 5
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define LED 2
DHT dht(DHTPIN, DHTTYPE);
//#define HEARTBEAT_INTERVAL 25000

uint64_t messageTimestamp = 0;
uint64_t heartbeatTimestamp = 0;
bool isConnected = false;
bool ledStatus = !false;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[WSc] Disconnected!\n");
            isConnected = false;
            break;
        case WStype_CONNECTED:
            {
                USE_SERIAL.printf("[WSc] Connected to url: %s\n",  payload);
                isConnected = true;
			    // send message to server when Connected
                // socket.io upgrade confirmation message (required)
				webSocket.sendTXT("5");
            }
            break;
        case WStype_TEXT:
          {
            USE_SERIAL.printf("[WSc] get text: %s\n", payload);
            //digitalWrite(LED, ledStatus);
            //ledStatus = !ledStatus;
            /*
            String text = ((const char *)&payload[0]);
            int pos = text.indexOf('{');
            String json = text.substring(pos,text.length()-1);

        const size_t BUFFER_SIZE =
             JSON_OBJECT_SIZE(10)
             + 200;
         DynamicJsonBuffer jsonBuffer(BUFFER_SIZE);

         JsonObject& root = jsonBuffer.parseObject(json);
         String msg = root["Ledi"];
         */
         //USE_SERIAL.printf("Ledi: %s\n", msg);
          }
            break;
        case WStype_BIN:
            USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
            hexdump(payload, length);

            // send data to server
            // webSocket.sendBIN(payload, length);
            break;
    }
}

void CreateJsonPayload(String &jsonString, float h, float t, float f, float hif, float hic, bool ledStat)
{
  String hum = String(h) + "%";
  String temp = String(t) + " *C " + String(f) + " *F";
  String heatIndex = String(hic) + " *C " + String(hif) + " *F";
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["Humidity"] = hum;
  root["Temperature"] = temp;
  root["Heat index"] = heatIndex;
  root["Led"] = ledStat;
  String buffer;
  root.printTo(buffer);
  jsonString = "42[\"tempSensor\","+buffer+"]";
}

void setup() {
    pinMode(LED, OUTPUT);
    digitalWrite(LED, ledStatus);
    // USE_SERIAL.begin(921600);
    USE_SERIAL.begin(9600);
    //Serial.setDebugOutput(true);
    USE_SERIAL.setDebugOutput(true);
    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();
    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }
    WiFiMulti.addAP(ssid, pass);

    //WiFi.disconnect();
    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
    }
    //webSocket.beginSocketIO("192.168.43.135", 3000);
    webSocket.beginSocketIO(host, port);
    //webSocket.setAuthorization("user", "Password"); // HTTP Basic Authorization
    webSocket.onEvent(webSocketEvent);
    dht.begin();
}

void loop() {
    webSocket.loop();
    if(isConnected) {
        uint64_t now = millis();
        if(now - messageTimestamp > MESSAGE_INTERVAL) {
            messageTimestamp = now;

            float h = dht.readHumidity();
            // Read temperature as Celsius (the default)
            float t = dht.readTemperature();
            // Read temperature as Fahrenheit (isFahrenheit = true)
            float f = dht.readTemperature(true);
            // Check if any reads failed and exit early (to try again).
            if (isnan(h) || isnan(t) || isnan(f)) {
              Serial.println("Failed to read from DHT sensor!");
              //return;
            }
            else{
              // Compute heat index in Fahrenheit (the default)
              float hif = dht.computeHeatIndex(f, h);
              // Compute heat index in Celsius (isFahreheit = false)
              float hic = dht.computeHeatIndex(t, h, false);

              String jsonString;
              CreateJsonPayload(jsonString, h, t, f, hif, hic, ledStatus);
              // example socket.io message with type "messageType" and JSON payload
              webSocket.sendTXT(jsonString);
            }
        }
    }
}
