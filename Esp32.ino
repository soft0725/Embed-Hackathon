#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include "DHT.h"

#define DHTPIN 33
#define DHTTYPE DHT11
#define led 15

const char* ssid = "bssm_free";
const char* password = "bssm_free";
const char* mqtt_server = "broker.mqtt-dashboard.com";

IPAddress server_addr(192, 168, 1, 100); // MySQL 서버의 IP 주소
char user[] = "root";                    // MySQL 사용자 이름
char password_db[] = "1234";             // MySQL 비밀번호
char default_db[] = "projectwinter";     // MySQL 데이터베이스 이름

WiFiClient wifiClient;                   // WiFi 클라이언트
MySQL_Connection conn((Client *)&wifiClient); // MySQL 연결
PubSubClient mqttClient(wifiClient);     // MQTT 클라이언트

DHT dht(DHTPIN, DHTTYPE);
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
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
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
  if(myled) {
    postForms.replace("$$led$$","ON");
  } else {
    postForms.replace("$$led$$","OFF");
  }
  server.send(200, "text/html", postForms);
}

void handleNockanda() {
  for(int i = 0; i < server.args(); i++){
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
  mqttClient.setServer(mqtt_server, 1883);

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/nockanda", handleNockanda);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void saveToDatabase(float temperature, float humidity) {
  if (conn.connect(server_addr, 3306, user, password_db, default_db)) {
    char query[128];
    sprintf(query, "INSERT INTO sensor_data (idx, temp, humi) VALUES (%d, %f, %f)", value, temperature, humidity);
    value += 1;
    MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
    cur_mem->execute(query);
    delete cur_mem;
    conn.close();
  } else {
    Serial.println("MySQL 연결 실패");
  }
}

void loop() {
  if (!mqttClient.connected()) {
    mqtt_reconnect();
  }
  mqttClient.loop();

  server.handleClient();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    saveToDatabase(t, h);
    snprintf (msg, 50, "%f", t);
    mqttClient.publish("nockanda/temp", msg);
    snprintf (msg, 50, "%f", h);
    mqttClient.publish("nockanda/humi", msg);
    snprintf (msg, 50, "%d", value);
    mqttClient.publish("nockanda/led", msg);
  }
}
