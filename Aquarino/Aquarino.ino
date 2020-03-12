/*
  Author: Nícholas Kegler
  Email: nicholasks@gmail.com
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FastLED.h>


#ifndef STASSID
#define STASSID "TestNetwork"
#define STAPSK  "P@ssW0RD0."
#endif

#define NUM_LEDS 34
#define LED_DT 14 // ~D5
#define LED_DT2 13 // ~D7
CRGB leds[NUM_LEDS];
CRGB leds2[NUM_LEDS];
bool ledState = false;
bool performAction = false;
int fade = 75;
int fade_up = 5;

const char* host = "aquarium";
const char* auth_header = "X-USER-TOKEN";
const char* auth_token  = "zdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4";
const char* ssid = STASSID;
const char* password = STAPSK;


ESP8266WebServer server(80);
const char* serverIndex = "<h2>Controlador aquario</h2>";


void _ledOn() {
  for (int i = 0; i <= NUM_LEDS; i++) {
    leds[i] = CRGB::MediumTurquoise;
    leds[i].maximizeBrightness( fade_up );
    leds2[i] = CRGB::PaleTurquoise;
    leds2[i].maximizeBrightness( fade_up );
  }
  FastLED.show();
  fade_up++;
  delay(60);
  
  if (fade_up >= 255) {
    performAction = false;
    fade_up = 5;
  }
}

void _ledOff() {
  for (int i = 0; i <= NUM_LEDS; i++) {
    leds[i].fadeToBlackBy( 12 );
    leds2[i].fadeToBlackBy( 12 );
  }
  FastLED.show();
  fade--;
  delay(300);
  if (fade <= 0) {
    performAction = false;
    fade = 75;
  }
  
}

void turnOnLeds() {
//  authenticate() && 
  if (isAvailable()) {
    ledState = true;
    performAction = true;
    const char* response = "\"success\": true, \"ledStatus\": \"on\"}";
    server.sendHeader("Connection", "close");
    server.send(202, "application/json", response);
  }
}

void turnOffLeds() {
//  authenticate() && 
  if (isAvailable()) {
    ledState = false;
    performAction = true;
    const char* response = "\"success\": true, \"ledStatus\": \"off\"}";
    server.sendHeader("Connection", "close");
    server.send(202, "application/json", response);
  }
}

void updateLeds() {
  if (performAction) {
    if (ledState)
      _ledOn();
    else
      _ledOff();
  }
}
bool isAvailable() {
  if (performAction) {
    server.sendHeader("Connection", "close");
    server.send(200, "application/json", "{\"success\": false, \"reason\": \"another action is pending\"}");
    return false;
  }
  return true;
}
bool authenticate(void) {
  if (server.hasHeader(auth_header)) {
    char token[100];
    strcpy(token, server.header(auth_header).c_str());
    if (strcmp(token, auth_token) == 0) {
      return true;
    } else {
      Serial.print("Wrong token: ");
      Serial.println(token);
      server.sendHeader("Connection", "close");
      server.send(401, "application/json", "{\"success\": false, \"reason\": \"token doesn't match\"}");
    }
  } else {
    server.sendHeader("Connection", "close");
    server.send(401, "application/json", "{\"success\": false, \"reason\": \"invalid token\"}");
  }
  return false;
}


void setup(void) {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting");
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    MDNS.begin(host);
    
    // ENDPOINT HANDLERS
    server.on("/leds/on", HTTP_GET, turnOnLeds);
    server.on("/leds/off", HTTP_GET, turnOffLeds);
    server.on("/", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", serverIndex);
    });

    // SET INCOMING HEADERS
    const char * headerkeys[] = {auth_header} ;
    size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
    server.collectHeaders(headerkeys, headerkeyssize );
    
    server.begin();
    MDNS.addService("http", "tcp", 80);

    Serial.printf("Ready! Open http://%s.local in your browser\n", host);

  
  
  } else {
    Serial.println("WiFi Failed");
  }
  FastLED.addLeds<WS2812B, LED_DT, GRB>(leds, NUM_LEDS);
  FastLED.addLeds<WS2812B, LED_DT2, GRB>(leds2, NUM_LEDS);
}

void loop(void) {
  server.handleClient();
  MDNS.update();
  updateLeds();
}
