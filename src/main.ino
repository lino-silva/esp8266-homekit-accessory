#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>

const uint DEBUG = 1;

const uint MAX_WIFI_CONNECT_TRY = 60;
const char* WLAN_SSID = "<redacted>";
const char* WLAN_PWD = "<redacted>";

ESP8266WebServer webServer(80);

bool OTAUpdate = false;
long OTATimeout = 60*1000;
long OTAend=0;

void setup_wifi() {
  int xTry = 0;
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PWD);

  while (WiFi.status() != WL_CONNECTED && ++xTry < MAX_WIFI_CONNECT_TRY) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED){
    Serial.println("Wifi Failed.. Restarting");
    delay(100);
    ESP.restart();
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void handleOTARoot() {
  if (DEBUG) Serial.println("GOT OTA cmd");
  OTAUpdate = true;
  OTAend = millis()+OTATimeout;
  webServer.send(200, "text/plain", "GO");
  if (DEBUG) Serial.println("Start OTA");
}

void bindOTAEvents(){
  ArduinoOTA.onStart([]() {

    if (DEBUG) Serial.println("Start updating ");
  });

  ArduinoOTA.onEnd([]() {
    if (DEBUG) Serial.println("\nEnd");
    OTAUpdate =false;
    delay(100);
    ESP.restart();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (DEBUG) Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    if (DEBUG) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    }

    delay(100);
    ESP.restart();
  });

  ArduinoOTA.setPort(9069);
  ArduinoOTA.setPassword("M4yTh3F0rc3B3W1thY0u");
  ArduinoOTA.begin();
}

void setup() {
  if (DEBUG) Serial.begin(115200);

  setup_wifi();
  webServer.on("/update", handleOTARoot);
  webServer.begin();
  if (DEBUG) Serial.println("HTTP server started");
  bindOTAEvents();
}

void loop(){
   if (OTAUpdate && millis() - OTAend>0) {

     ArduinoOTA.handle();
   } else {

    OTAUpdate=false;
    webServer.handleClient();
  }
}
