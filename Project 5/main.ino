#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// WiFi
const char* ssid = "Interstellar_Fi";
const char* password = "Ritu195203";

// ThingsBoard
const char* mqtt_server = "mqtt.eu.thingsboard.cloud";
const char* token = "ud863cnku0a9n4nai6f4";

// Pins
#define SOIL_PIN A0
#define RELAY_PIN D1
#define DHTPIN D2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

int soilThreshold = 600; // adjust based on your sensor

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // relay OFF

  dht.begin();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting...");

    if (client.connect(token, token, NULL)) {
      Serial.println("Connected!");
    } else {
      Serial.print("Failed=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  int soilValue = analogRead(SOIL_PIN);

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT Error");
    return;
  }

  //  Irrigation Logic
  bool pumpState;

  if (soilValue > soilThreshold) {
    digitalWrite(RELAY_PIN, LOW); // ON
    pumpState = true;
  } else {
    digitalWrite(RELAY_PIN, HIGH); // OFF
    pumpState = false;
  }

  // JSON Data
  String payload = "{";
  payload += "\"soil\":";
  payload += soilValue;
  payload += ",";
  payload += "\"temperature\":";
  payload += temp;
  payload += ",";
  payload += "\"humidity\":";
  payload += hum;
  payload += ",";
  payload += "\"pump\":";
  payload += pumpState ? "true" : "false";
  payload += "}";

  client.publish("v1/devices/me/telemetry", payload.c_str());

  Serial.println(payload);

  delay(5000);
}