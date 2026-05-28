#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi Configuration Settings
#define WIFI_SSID "YOUR_WIFI_SSID"          // SENSITIVE INFO REMOVED: Insert your Wi-Fi SSID here
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"  // SENSITIVE INFO REMOVED: Insert your Wi-Fi Password here

// ThingsBoard Cloud MQTT Broker Credentials
#define TOKEN "YOUR_THINGSBOARD_TOKEN"      // SENSITIVE INFO REMOVED: Insert your ThingsBoard Device Access Token here
#define THINGSBOARD_SERVER "mqtt.thingsboard.cloud"

// Hardware Pin Definitions
#define LDR_PIN A0   // Analog input pin connected to the LDR sensor
#define LED_PIN D1   // Digital output pin connected to the indicator LED

// Initialize Network and MQTT Client instances
WiFiClient espClient;
PubSubClient client(espClient);

// Darkness Threshold (0 to 1023): Higher means darker ambient environment required to trigger LED
int threshold = 700;

/**
 * Establishes connection to the local Wi-Fi Access Point
 */
void connectWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("."); // Keep printing dots until connected
  }

  Serial.println("\nWiFi Connected");
}

/**
 * Handles automatic MQTT reconnection to ThingsBoard Server if connection drops
 */
void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to ThingsBoard MQTT Broker...");

    // THINGSBOARD REQUIREMENT: The access token must be passed as the Username string
    if (client.connect("NodeMCU_LDR", TOKEN, NULL)) {
      Serial.println("Connected to ThingsBoard Successfully!");
    } else {
      Serial.print("Connection Failed, Client State code: ");
      Serial.println(client.state());
      Serial.println("Retrying connection handshake in 2 seconds...");
      delay(2000);
    }
  }
}

void setup() {
  // Initialize hardware serial baud rate for local console printouts
  Serial.begin(115200);

  // Configure digital pin directional states
  pinMode(LED_PIN, OUTPUT);

  // Initialize network interfaces
  connectWiFi();

  // Assign target MQTT server destination over standard unencrypted port (1883)
  client.setServer(THINGSBOARD_SERVER, 1883);
}

void loop() {
  // Ensure network and cloud sessions remain active before running hardware loop
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Read raw voltage from LDR sensor on the built-in Analog-to-Digital Converter (ADC)
  int ldrValue = analogRead(LDR_PIN);

  Serial.print("LDR Value: ");
  Serial.println(ldrValue);

  // --- CONDITION 1: ENVIRONMENT IS DARK ---
  if (ldrValue > threshold) {
    // Turn local hardware LED on
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Status: LED ON");

    // Package telemetry fields into ThingsBoard compliant JSON structure
    String payload = "{\"led\":1,\"ldr\":";
    payload += ldrValue;
    payload += "}";

    // Publish data packet string to ThingsBoard core client telemetry topic
    client.publish("v1/devices/me/telemetry", payload.c_str());
  }
  // --- CONDITION 2: ENVIRONMENT IS BRIGHT ---
  else {
    // Turn local hardware LED off
    digitalWrite(LED_PIN, LOW);
    Serial.println("Status: LED OFF");

    // Package telemetry fields into ThingsBoard compliant JSON structure
    String payload = "{\"led\":0,\"ldr\":";
    payload += ldrValue;
    payload += "}";

    // Publish data packet string to ThingsBoard core client telemetry topic
    client.publish("v1/devices/me/telemetry", payload.c_str());
  }

  // Enforce a structured polling rate delay of 1 second per cycle
  delay(1000);
}