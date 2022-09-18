/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp8266-nodemcu-websocket-server-arduino/
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include "index_html.h"

#define MIN_MOTOR_VALUE 130
#define MAX_PWM_VAL 255
#define PWMA 5  //Right side 
#define PWMB 4  //Left side 
#define DA 0    //Right reverse 
#define DB 2    //Left reverse //also LED pin


// Replace with your network credentials
const char* ssid = "YOUR SID";
const char* password = "YOUR Password";

int x = 0;
int y = 0;
float z = 0;  //magnitude

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void notifyClients() {
  ws.textAll(String("somevalue"));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    
//// For debugging
//    Serial.print("Incoming data: ");
//    Serial.println((char*)data);

    String tmp = (char*)data;
    int index = tmp.indexOf(",");
    if (index > -1) {
      x = tmp.substring(0, index).toInt();
      y = tmp.substring(index + 1, tmp.length()).toInt();
      z = sqrt(x*x + y*y);
      //// For debugging
       Serial.println(x);
       Serial.println(y);
       Serial.println(z);
    }
    
//// Send data back to the client
//    if (strcmp((char*)data, "toggle") == 0) {
//      ledState = !ledState;
//      notifyClients();
//    }
  calc();
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var){
  return "";
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Initialize LittleFS
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  pinMode(PWMA, OUTPUT);  //ENA -- which is being used as a PWM  
  pinMode(PWMB, OUTPUT);  //ENB -- which is being used as a PWM 
  pinMode(DA, OUTPUT); 
  pinMode(DB, OUTPUT); 
    
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  // Open a web 
  Serial.print("Open a web browser to http://");
  Serial.println(WiFi.localIP());

  initWebSocket();

  Serial.println("initWebSocket");
  
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/favicon.ico", "image/png");
  });

  // Route for root / web page
  server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/hello.html", String(), false, processor);
  });

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Start server
  server.begin();
}

void calc() {
    int right = 0;
  int left = 0;
  ws.cleanupClients();
  if (y > 30) { //forward

    if ( x >= 0){
      left = (int)(z/100.0 * 255);
      right = (int)(z/100.0 * (100.0 - (float)x)/100.0 * MAX_PWM_VAL);
      if (right < MIN_MOTOR_VALUE) { // prevents from turning too sharp
        right = MIN_MOTOR_VALUE;
      }
    } else {
      left = (int)(z/100.0 * (100.0 + (float)x)/100.0 * MAX_PWM_VAL);
      if (left < MIN_MOTOR_VALUE) { // prevents from turning too sharp
        left = MIN_MOTOR_VALUE;
      }
      right = (int)(z/100.0 * 255);
    }

    analogWrite(PWMA, left);
    analogWrite(PWMB, right);
    digitalWrite(DA, LOW); 
    digitalWrite(DB, HIGH); 
  } else  if (y < -30) { //backward
    digitalWrite(PWMA, HIGH);
    digitalWrite(PWMB, HIGH);
    digitalWrite(DA, HIGH); 
    digitalWrite(DB, LOW); 
  } else {
    digitalWrite(PWMA, LOW);
    digitalWrite(PWMB, LOW);
  }
}

void loop() {
}
