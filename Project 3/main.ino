/**
 * @file Project 3/main.ino
 * @brief IoT Motion Detection and Alarm System via ThingsBoard MQTT
 * * DESCRIPTION:
 * This firmware connects an ESP8266 (NodeMCU) to Wi-Fi and utilizes a PIR motion
 * sensor to detect physical movement. When intrusion/motion is detected, it triggers
 * a local audible buzzer alarm and publishes a real-time binary state telemetry
 * payload ("{\"motion\":1}" or "{\"motion\":0}") to the ThingsBoard IoT Cloud platform.
 * To optimize bandwidth, telemetry is strictly published upon state change.
 * * EXTERNAL LIBRARIES REQUIRED:
 * 1. PubSubClient by Nick O'Leary (For MQTT broker communication)
 * 2. Built-in ESP8266WiFi library
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi Configuration (REMOVED SENSITIVE INFO - Replace with your network details)
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ThingsBoard Cloud MQTT Configuration
#define THINGSBOARD_SERVER "mqtt.thingsboard.cloud"
const int MQTT_PORT = 1883;
const char* TELEMETRY_TOPIC = "v1/devices/me/telemetry";

// Device Access Token from ThingsBoard (REMOVED SENSITIVE INFO - Replace with your device token)
#define TOKEN "YOUR_THINGSBOARD_ACCESS_TOKEN"

// Hardware Pin Definitions
#define PIR_PIN D5      // Pin connected to PIR Sensor output (GPIO 14)
#define BUZZER_PIN D6    // Pin connected to Buzzer positive terminal (GPIO 12)

// Global Objects and Tracking Variables
WiFiClient espClient;
PubSubClient client(espClient);

int lastMotionState = -1; // Keeps track of previous state to avoid telemetry spamming

/**
 * @brief Initializes connection to the local Wi-Fi access point
 */
void connectWiFi() {
  Serial.print("\nConnecting to Wi-Fi Network: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected successfully!");
  Serial.print("NodeMCU IP Address: ");
  Serial.println(WiFi.localIP());
}

/**
 * @brief Manages connection stability and handles reconnections to the ThingsBoard MQTT broker
 */
void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to ThingsBoard Broker...");

    // ThingsBoard requires the Device Access Token to be passed as the MQTT Username
    if (client.connect(TOKEN, TOKEN, NULL)) {
      Serial.println("Connected to ThingsBoard successfully!");
    } else {
      Serial.print("Connection failed, rc=");
      Serial.print(client.state());
      Serial.println(". Retrying in 2 seconds...");
      delay(2000); // Guard delay to prevent aggressive reconnections
    }
  }
}

void setup() {
  // Initialize Serial Monitor for system diagnostics
  Serial.begin(115200);

  // Configure hardware peripheral pins
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off initially

  // PIR sensors take time to stabilize their infrared image profile when powered up
  Serial.println("Awaiting PIR Sensor warm-up/calibration (30 Seconds)...");
  delay(30000);
  Serial.println("Calibration complete. System Active.");

  // Establish Network and Server Routes
  connectWiFi();
  client.setServer(THINGSBOARD_SERVER, MQTT_PORT);
}

void loop() {
  // Maintain live MQTT connection
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Read current input from the motion sensor
  int currentMotionState = digitalRead(PIR_PIN);

  // Execute logic ONLY when the state changes (Motion starts OR motion stops)
  if (currentMotionState != lastMotionState) {

    if (currentMotionState == HIGH) {
      // Intrusion detected
      digitalWrite(BUZZER_PIN, HIGH);
      Serial.println("Alert: Motion Detected!");

      // Send active state to Cloud
      client.publish(TELEMETRY_TOPIC, "{\"motion\":1}");
      Serial.println("Telemetry Transmitted -> Motion: 1");
    }
    else {
      // Area cleared
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("Status: No Motion / Secure");

      // Send secure state to Cloud
      client.publish(TELEMETRY_TOPIC, "{\"motion\":0}");
      Serial.println("Telemetry Transmitted -> Motion: 0");
    }

    // Update state tracker
    lastMotionState = currentMotionState;
  }

  // Small stable polling delay
  delay(100);
}