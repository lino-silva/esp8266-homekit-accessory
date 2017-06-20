#include <Arduino.h>
#include <ArduinoOTA.h>

#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

#include <WiFiUdp.h>

const uint DEBUG = 1;

const uint MAX_WIFI_CONNECT_TRY = 60;
const char* WLAN_SSID = "<redacted>";
const char* WLAN_PWD = "<redacted>";

const uint OTA_PORT = 0;
const char* OTA_PASSWORD = "<redacted>";

ESP8266WebServer webServer(80);

bool OTAUpdate = false;
long OTATimeout = 60*1000;
long OTAend=0;

void setup_wifi() {
  int xTry = 0;
  delay(10);

#if defined(DEBUG)
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
#endif

  WiFi.begin(WLAN_SSID, WLAN_PWD);

  while (WiFi.status() != WL_CONNECTED && ++xTry < MAX_WIFI_CONNECT_TRY) {
    delay(500);
#if defined(DEBUG)
      Serial.print(".");
#endif
  }

  if (WiFi.status() != WL_CONNECTED){
#if defined(DEBUG)
      Serial.println("Wifi Failed.. Restarting");
#endif
    delay(100);
    ESP.restart();
  }

#if defined(DEBUG)
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
#endif
}

void handleOTARoot() {
#if defined(DEBUG)
    Serial.println("GOT OTA cmd");
#endif
  OTAUpdate = true;
  OTAend = millis()+OTATimeout;
  webServer.send(200, "text/plain", "GO");
#if defined(DEBUG)
    Serial.println("Start OTA");
#endif
}

void bindOTAEvents(){
  ArduinoOTA.onStart([]() {

#if defined(DEBUG)
    Serial.println("Start updating ");
#endif
  });

  ArduinoOTA.onEnd([]() {
#if defined(DEBUG)
    Serial.println("\nEnd");
#endif
    OTAUpdate =false;
    delay(100);
    ESP.restart();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  #if defined(DEBUG)
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  #endif
  });

  ArduinoOTA.onError([](ota_error_t error) {
#if defined(DEBUG)
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
#endif

    delay(100);
    ESP.restart();
  });

  ArduinoOTA.setPort(OTA_PORT);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();
}

void setup_mdns() {

  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin("esp8266")) {

#if defined(DEBUG)
    Serial.println("Error setting up MDNS responder!");
#endif

    while(1) {
      delay(1000);
    }
  }

#if defined(DEBUG)
  Serial.println("mDNS responder started");
#endif
}

void setup() {
#if defined(DEBUG)
  Serial.begin(115200);
#endif
  setup_wifi();
  setup_mdns();
  webServer.on("/update", handleOTARoot);
  webServer.begin();
#if defined(DEBUG)
  Serial.println("HTTP server started");
#endif
  bindOTAEvents();
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}

void loop(){
   if (OTAUpdate && millis() - OTAend > 0) {

     ArduinoOTA.handle();
   } else {

    OTAUpdate=false;
    webServer.handleClient();
  }
}
