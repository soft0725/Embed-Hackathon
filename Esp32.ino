/*
 C#109-1 사물인터넷보드(wemos d1r1)에 온습도센서(DHT11)을 D3에 연결했다!
 온습도센서에서 온도와 습도를 측정해서 MQTT BROKER로 Publish 해보자!
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include "DHT.h"

#define DHTPIN 33
#define DHTTYPE DHT11 
#define led 15

// Update these with values suitable for your network.
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "bssm_free";
const char* password = "bssm_free";
const char* mqtt_server = "broker.mqtt-dashboard.com";

WebServer server(80);

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);

  server.on("/nockanda", handleNockanda);

  server.onNotFound(handleNotFound);

  server.begin();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
//    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
//    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      //client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void handleRoot() {
  //response
  String postForms = "<html>\
  <head>\
    <meta charset=\"utf-8\">\
    <title>원격 시동 걸기</title>\
  </head>\
  <body>\
    현재 시동 상태 $$led$$\
    <form method=\"post\" action=\"/nockanda\">\
      <br>0: 시동 off, 1: 시동 on<br>\
      <input type=\"text\" name=\"led\" value=\"\"><br>\
      <input type=\"submit\" value=\"전송\">\
    </form>\
  </body>\
</html>";

  bool myled = digitalRead(led);
  if(myled){
    //ON
    postForms.replace("$$led$$","ON");
  }else{
    //OFF
    postForms.replace("$$led$$","OFF");
  }
  server.send(200, "text/html", postForms);
}
void handleNockanda() {
  //response
  //server.args(); 서버로 post방식으로 넘어온 변수의 갯수
  //server.argName(~) : 변수이름
  //server.arg(~) : 값
  
  for(int i = 0;i<server.args();i++){
    Serial.print(server.argName(i));
    Serial.print(",");
    Serial.print(server.arg(i));
    Serial.println();
    if(server.argName(i) == "led"){
      if(server.arg(i) == "0"){
        digitalWrite(led, LOW);
      }else if(server.arg(i) == "1"){
       digitalWrite(led, HIGH); 
      }
    }
  }

  //홈페이지로 리다이렉션
  String res = "<meta http-equiv=\"refresh\" content=\"0; url=http://"+WiFi.localIP().toString()+"\">";

  server.send(200, "text/html", res);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  server.send(404, "text/plain", message);
}

void setup() {
  pinMode(led,OUTPUT);
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  server.handleClient();
  delay(2);//allow the cpu to switch to other tasks

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    //++value;
    //snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    //Serial.print("Publish message: ");
    //Serial.println(msg);
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
  
    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.println(t);
    snprintf (msg, MSG_BUFFER_SIZE, "%f", t);
    client.publish("nockanda/temp", msg);
    snprintf (msg, MSG_BUFFER_SIZE, "%f", h);
    client.publish("nockanda/humi", msg);
  }
}
