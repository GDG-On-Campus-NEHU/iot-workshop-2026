#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi and MQTT Credentials Setup
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_topic = "yourunique/parking/status"; // Change this to be unique!

// Pin Definitions
const int trigPin = 5;   // D1 on NodeMCU
const int echoPin = 4;   // D2 on NodeMCU
const int greenLed = 14; // D5 on NodeMCU
const int redLed = 12;   // D6 on NodeMCU (Double-check if your red wire is on D6 or D7!)

// Parking Threshold (in centimeters)
const int PARKING_THRESHOLD = 20;

// Variables
long duration;
float distanceCm;
String currentStatus = "UNKNOWN";

// Non-blocking timer for MQTT publishing (every 2 seconds)
unsigned long lastMqttPublish = 0;
const long publishInterval = 2000;

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
    Serial.print("Attempting MQTT connection to broker...");
    // Generate a random client ID to prevent collision on the public broker
    String clientId = "NodeMCU-ParkingSensor-";
    clientId += String(random(0, 0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println(" connected!");
    } else {
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.println(" -> Retrying connection in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  // Initialize Serial Monitor for debugging
  Serial.begin(115200);
  Serial.println("\n--- Smart Ultrasonic Parking Assistant (MQTT Enabled) ---");

  // Define pin modes
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT); // If keeping no resistors, you can change this to INPUT_PULLUP
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
  // Ensure we maintain a solid connection to the MQTT broker
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

  // 6. Non-blocking MQTT Data Transmission
  unsigned long currentMillis = millis();
  if (currentMillis - lastMqttPublish >= publishInterval) {
    lastMqttPublish = currentMillis;

    // Create a data payload string combining the distance and state
    String payload = "{\"distance\":" + String(distanceCm, 1) + ",\"status\":\"" + currentStatus + "\"}";

    // Publish payload to cloud broker
    if (client.publish(mqtt_topic, payload.c_str())) {
      Serial.print(">> Cloud Telemetry Streamed: ");
      Serial.println(payload);
    } else {
      Serial.println(">> Cloud Telemetry stream failed.");
    }
  }

  // Small delay for raw sensor polling stability
  delay(100);
}