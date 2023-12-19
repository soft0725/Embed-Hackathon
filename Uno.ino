#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include "DHT.h"

#define DHTPIN 33
#define DHTTYPE DHT11
#define led 15

const char* ssid = "bssm_free";
const char* password = "bssm_free";
const char* mqtt_server = "broker.mqtt-dashboard.com";

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);

unsigned long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqtt_reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void handleRoot() {
  String postForms = "<html>\
  <head>\
    <meta charset=\"utf-8\">\
    <title>원격 시동 걸기</title>\
  </head>\
  <body>\
    현재 시동 상태 $$led$$\
    <br>온도: $$temp$$ °C<br>습도: $$humi$$ %<br>\
    <form method=\"post\" action=\"/nockanda\">\
      <br>0: 시동 off, 1: 시동 on<br>\
      <input type=\"text\" name=\"led\" value=\"\"><br>\
      <input type=\"submit\" value=\"전송\">\
    </form>\
  </body>\
</html>";

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  postForms.replace("$$temp$$", String(t));
  postForms.replace("$$humi$$", String(h));

  bool myled = digitalRead(led);
  if(myled){
    postForms.replace("$$led$$","ON");
  }else{
    postForms.replace("$$led$$","OFF");
  }
  server.send(200, "text/html", postForms);
}

void handleNockanda() {
  for(int i = 0;i<server.args();i++){
    if(server.argName(i) == "led"){
      if(server.arg(i) == "0"){
        value = 0;
        digitalWrite(led, LOW);
      }else if(server.arg(i) == "1"){
        value = 1;
        digitalWrite(led, HIGH); 
      }
    }
  }
  String res = "<meta http-equiv=\"refresh\" content=\"0; url=http://"+WiFi.localIP().toString()+"\">";
  server.send(200, "text/html", res);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  server.send(404, "text/plain", message);
}

void setup() {
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/nockanda", handleNockanda);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  if (!client.connected()) {
    mqtt_reconnect();
  }
  client.loop();

  server.handleClient();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    snprintf (msg, 50, "%f", t);
    client.publish("nockanda/temp", msg);
    snprintf (msg, 50, "%f", h);
    client.publish("nockanda/humi", msg);
    snprintf (msg, 50, "%d", value);
    client.publish("nockanda/led", msg);
  }
}
