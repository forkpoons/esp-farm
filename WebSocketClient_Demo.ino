#include <ESP8266WiFi.h>
#include <WebSocketClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include "DHT.h"

const char* ssid =     "huawei-015g";     //  SSID wi-fi роутера
const char* password = "19813120"; // Пароль от wi-fi
char path[] = "/api/echo";
char host[] = "farmpl.aidew.online";
//char host[] = "192.168.0.120";

#define OTAUSER         "admin"    // Логин для входа в OTA
#define OTAPASSWORD     "admin"    // Пароль для входа в ОТА
#define OTAPATH         "/firmware"// Путь, который будем дописывать после ip адреса в браузере.
#define SERVERPORT      80         // Порт для входа, он стандартный 80 это порт http
#define DHTPIN          0
#define DHTTYPE         DHT11   // DHT 11
#define RELEY1PIN       2
#define DEVICENAME      "farm"

DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer HttpServer(SERVERPORT);
ESP8266HTTPUpdateServer httpUpdater;
WebSocketClient webSocketClient;
float tempMin = 10.0;
float tempMax = 15.0;
bool work = false;
float tempSend = 100.0;
bool starts = false;
int timing = 0;
// Use WiFiClient class to create TCP connections
WiFiClient client;

void setup() {
  Serial.begin(115200);
  delay(10);
  pinMode(RELEY1PIN, OUTPUT);
  digitalWrite(RELEY1PIN, LOW);
  delay(500);
  digitalWrite(RELEY1PIN, HIGH);
  delay(500);
  
  //Serial.println();
  //Serial.println();
  //Serial.print("Connecting to ");
  //Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  delay(4000);
  if (WiFi.status() == WL_CONNECTED){
    Start();
  }
  dht.begin();
}

void Start() {
  httpUpdater.setup(&HttpServer, OTAPATH, OTAUSER, OTAPASSWORD);
  HttpServer.onNotFound(handleNotFound);
  HttpServer.on("/temp", handleTemp);
  HttpServer.begin();
  socketConnect();
  starts = true;
}

void loop() {
  float temp = dht.readTemperature();
  if (temp < tempMin && !work) {
    relay(true);
  } else if (temp > tempMax && work) {
    relay(false);
  }
  
  //Serial.println(String(temp) + "|" + String(tempMin));
  if (WiFi.status() == WL_CONNECTED){
    if(!starts){
      Start();
    }
    
    String data;
    HttpServer.handleClient(); 
    if (client.connected()) {
      //Serial.println("2 ");
      webSocketClient.getData(data);
      if (data.length() > 0 && data == "ping") {
        webSocketClient.sendData("pong");
      } else if (data.length() == 8) {
        String sub_S = data.substring(0,2);
        if(sub_S == "##"){
          tempMin = data.substring(2,3).toInt();
          tempMax = data.substring(4,5).toInt();
          webSocketClient.sendData(String("##") + String(tempMin)  + "|" + String(tempMax));
          //Serial.print("Received data: ");
          //Serial.println(data);
        }
      }
      if(abs(temp - tempSend) > 0.5){
        webSocketClient.sendData(String("##") + DEVICENAME  + "|temp|" + String(temp) + "#");
        tempSend = temp;
      }
      if (millis() - timing > 120000){ // Вместо 10000 подставьте нужное вам значение паузы 
        timing = millis();
        webSocketClient.sendData("ping");
      }
    } else {
      //Serial.println("Client disconnected.");
      socketConnect();
    }
  }  
}

void relay(bool w) {
  if (w == true) {
    //Serial.println(String(w));
    digitalWrite(RELEY1PIN, HIGH);
    work = true;
  } else {
    digitalWrite(RELEY1PIN, LOW);
    work = false;
  }
  if (client.connected()) {
    //Serial.println("qweqweq" + String(work));
    webSocketClient.sendData(String("##") + DEVICENAME + "|action|" + String(work) + "#");
  }
}

void socketConnect() {
  if (client.connect(host, 80)) {
    webSocketClient.path = path;
    webSocketClient.host = host;
    webSocketClient.handshake(client);
  
    /*
    if (webSocketClient.handshake(client)) {
      //Serial.println("Handshake successful");
    } else {
      //Serial.println("Handshake failed.");
    }
    */
    //Serial.println("Connected");
  } else {
    //Serial.println("Connection failed.");
  }
}

void handleNotFound() {
  HttpServer.send(404, "text/plain", "404: Not found");
}

void handleTemp() {
  HttpServer.send(200, "text/plain", String("Temp:") + String(dht.readTemperature()));
}
