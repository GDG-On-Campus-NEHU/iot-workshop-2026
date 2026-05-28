#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi Configuration Setup
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ThingsBoard Cloud MQTT Broker Configuration
const char* mqtt_server = "mqtt.eu.thingsboard.cloud"; // Alter server if using thingsboard.cloud global region
const char* token = "YOUR_THINGSBOARD_TOKEN";          // Device Access Token acting as MQTT credentials
const char* mqtt_topic = "v1/devices/me/telemetry";    // Strict ThingsBoard endpoint path

// Pin Definitions (Direct GPIO mapping indices)
const int trigPin = 5;   // D1 on NodeMCU
const int echoPin = 4;   // D2 on NodeMCU
const int greenLed = 14; // D5 on NodeMCU
const int redLed = 12;   // D6 on NodeMCU

// Parking Threshold (in centimeters)
const int PARKING_THRESHOLD = 20;

// Variables
long duration;
float distanceCm;
String currentStatus = "UNKNOWN";

// Non-blocking timer for ThingsBoard telemetry intervals (every 2.5 seconds)
unsigned long lastMqttPublish = 0;
const long publishInterval = 2500;

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to network: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi successfully connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection to ThingsBoard Server...");

    // THINGSBOARD PROTOCOL REQUIREMENT: Token passes as username, password remains NULL
    if (client.connect(token, token, NULL)) {
      Serial.println(" successfully authenticated and connected!");
    } else {
      Serial.print(" authentication failed, rc=");
      Serial.print(client.state());
      Serial.println(" -> Retrying interface in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  // Initialize Serial Monitor for debugging
  Serial.begin(115200);
  Serial.println("\n--- Smart Ultrasonic Parking Assistant (ThingsBoard Configured) ---");

  // Define pin modes
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT); // Change to INPUT_PULLUP if bypassing resistors for raw diagnostics
  pinMode(greenLed, OUTPUT);
  pinMode(redLed, OUTPUT);

  // Flash LEDs once at startup to verify they work
  digitalWrite(greenLed, HIGH);
  digitalWrite(redLed, HIGH);
  delay(500);
  digitalWrite(greenLed, LOW);
  digitalWrite(redLed, LOW);

  // Network initialization
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  // Ensure we maintain a solid connection to the ThingsBoard broker
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 1. Trigger the sensor to send a sound wave
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // 2. Read the echo pulse duration (in microseconds)
  duration = pulseIn(echoPin, HIGH);

  // 3. Calculate distance in centimeters
  distanceCm = (duration * 0.0343) / 2;

  // Check for sensor reading errors
  if (duration == 0 || distanceCm > 400) {
    Serial.println("Error: No pulse received. Check sensor wiring.");
    currentStatus = "ERROR";

    // Blink both LEDs rapidly to indicate a physical hardware error
    digitalWrite(greenLed, HIGH);
    digitalWrite(redLed, HIGH);
    delay(100);
    digitalWrite(greenLed, LOW);
    digitalWrite(redLed, LOW);
    delay(100);
    return;
  }

  // 4. Print results to Serial Monitor locally
  Serial.print("Distance: ");
  Serial.print(distanceCm);
  Serial.print(" cm  |  Status: ");

  // 5. LED Control Logic
  if (distanceCm < PARKING_THRESHOLD) {
    // Something is too close! Turn on Red, turn off Green
    digitalWrite(redLed, HIGH);
    digitalWrite(greenLed, LOW);
    Serial.println("[ OCCUPIED / TOO CLOSE ]");
    currentStatus = "OCCUPIED";
  } else {
    // Space is clear! Turn on Green, turn off Red
    digitalWrite(greenLed, HIGH);
    digitalWrite(redLed, LOW);
    Serial.println("[ AVAILABLE ]");
    currentStatus = "AVAILABLE";
  }

  // 6. Non-blocking ThingsBoard Telemetry Data Transmission
  unsigned long currentMillis = millis();
  if (currentMillis - lastMqttPublish >= publishInterval) {
    lastMqttPublish = currentMillis;

    // Create a strict JSON data payload matching ThingsBoard attributes schema
    String payload = "{";
    payload += "\"distance\";";
    payload += String(distanceCm, 1);
    payload += ",";
    payload += "\"status\":\"";
    payload += currentStatus;
    payload += "\"";
    payload += "}";

    // Publish telemetry payload to cloud broker endpoint
    if (client.publish(mqtt_topic, payload.c_str())) {
      Serial.print(">> ThingsBoard Telemetry Streamed: ");
      Serial.println(payload);
    } else {
      Serial.println(">> ThingsBoard transmission packet dropped.");
    }
  }

  // Small delay for raw sensor polling stability
  delay(100);
}