#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h> // REQUIRED: Install "ArduinoJson" by Benoit Blanchon via Library Manager

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "mqtt.thingsboard.cloud";
const char* token = "YOUR_THINGSBOARD_TOKEN";

#define SOIL_PIN A0
#define RELAY_PIN D1
#define DHTPIN D2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

int soilThreshold = 600;

// OVERRIDE VARIABLES FOR RPC
bool cloudOverrideActive = false;
bool cloudPumpState = false;

// Non-blocking telemetry timer variables
unsigned long lastTelemetryTime = 0;
const unsigned long telemetryInterval = 5000;

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Default: Relay OFF (Active Low)

  dht.begin();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
}

/**
 * MQTT Callback Routine: Intercepts downstream server RPC requests
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("\n[RPC Received] On Topic: ");
  Serial.println(topic);

  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Payload: ");
  Serial.println(message);

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.print("JSON Parsing Failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* method = doc["method"];

  if (method != NULL) {
    if (strcmp(method, "setPumpState") == 0) {
      cloudOverrideActive = true;
      cloudPumpState = doc["params"].as<bool>();
      Serial.print("Cloud Override Engaged! Targeted Pump State: ");
      Serial.println(cloudPumpState ? "ON" : "OFF");
    }
    else if (strcmp(method, "releaseControl") == 0) {
      cloudOverrideActive = false;
      Serial.println("Cloud Override Disengaged. Reverting to local automatic automation.");
    }
  }

  String topicStr = String(topic);
  if (topicStr.startsWith("v1/devices/me/rpc/request/")) {
    String requestId = topicStr.substring(26);
    String responseTopic = "v1/devices/me/rpc/response/" + requestId;
    String responsePayload = "{\"success\":true,\"overrideActive\":" + String(cloudOverrideActive ? "true" : "false") + "}";
    client.publish(responseTopic.c_str(), responsePayload.c_str());
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting ThingsBoard MQTT connection...");
    if (client.connect(token, token, NULL)) {
      Serial.println("Connected!");
      client.subscribe("v1/devices/me/rpc/request/+");
      Serial.println("Subscribed to Server-Side RPC channel.");
    } else {
      Serial.print("Failed, rc=");
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

  bool activePumpState;

  if (cloudOverrideActive) {
    digitalWrite(RELAY_PIN, cloudPumpState ? LOW : HIGH);
    activePumpState = cloudPumpState;
  }
  else {
    if (soilValue > soilThreshold) {
      digitalWrite(RELAY_PIN, LOW);
      activePumpState = true;
    } else {
      digitalWrite(RELAY_PIN, HIGH);
      activePumpState = false;
    }
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastTelemetryTime >= telemetryInterval) {
    lastTelemetryTime = currentMillis;

    if (isnan(temp) || isnan(hum)) {
      Serial.println("DHT Sensor Read Error!");
      return;
    }

    String payload = "{";
    payload += "\"soil\":"; payload += soilValue; payload += ",";
    payload += "\"temperature\":"; payload += temp; payload += ",";
    payload += "\"humidity\":"; payload += hum; payload += ",";
    payload += "\"pump\":"; payload += activePumpState ? "true" : "false"; payload += ",";
    payload += "\"override\":"; payload += cloudOverrideActive ? "true" : "false";
    payload += "}";

    client.publish("v1/devices/me/telemetry", payload.c_str());
    Serial.println(payload);
  }
}