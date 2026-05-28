/**
 * @file Project 1/main.ino
 * @brief IoT Environmental Monitoring via ThingsBoard MQTT
 * * DESCRIPTION:
 * This firmware connects an ESP8266 microcontroller to a local Wi-Fi network and
 * establishes an MQTT connection with the ThingsBoard IoT cloud platform. It reads
 * temperature and humidity telemetry data from a DHT11 sensor every 5 seconds,
 * packages the metrics into a formatted JSON string, and securely publishes them
 * to the default ThingsBoard telemetry endpoint.
 * * EXTERNAL LIBRARIES REQUIRED:
 * 1. PubSubClient by Nick O'Leary (For MQTT communication)
 * 2. DHT sensor library by Adafruit (For interacting with the DHT11 sensor)
 * 3. Adafruit Unified Sensor by Adafruit (Underlying dependency for the DHT library)
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// WiFi Credentials (REMOVED SENSITIVE INFO - Replace with your network details)
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ThingsBoard MQTT Server configurations
const char* mqtt_server = "mqtt.eu.thingsboard.cloud";
const int mqtt_port = 1883;
const char* telemetry_topic = "v1/devices/me/telemetry";

// Device Access Token from ThingsBoard (REMOVED SENSITIVE INFO - Replace with your device token)
const char* token = "YOUR_THINGSBOARD_ACCESS_TOKEN";

// DHT Sensor Hardware Setup
#define DHTPIN D2        // Pin where the DHT11 data line is connected (GPIO 4)
#define DHTTYPE DHT11    // Defining the specific sensor model
DHT dht(DHTPIN, DHTTYPE);

// Instantiate network clients
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // Initialize hardware serial communication for monitoring/debugging
  Serial.begin(115200);

  // Initialize the DHT11 sensor
  dht.begin();

  // Begin the connection sequence to the Wi-Fi network
  Serial.print("Connecting to Wi-Fi Network: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // Block execution until Wi-Fi connection is securely established
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected successfully!");
  Serial.print("Local IP Address: ");
  Serial.println(WiFi.localIP());

  // Assign the MQTT server and port details to our pub-sub client
  client.setServer(mqtt_server, mqtt_port);
}

/**
 * @brief Handles connection and reconnection management to the ThingsBoard MQTT broker
 */
void reconnect() {
  // Loop continuously until a connection is successfully re-established
  while (!client.connected()) {
    Serial.print("Attempting connection to ThingsBoard broker...");

    // ThingsBoard expects the Device Access Token passed as both Username and Password parameters
    if (client.connect(token, token, NULL)) {
      Serial.println(" Connected successfully!");
    } else {
      Serial.print(" Failed connection, rc=");
      Serial.print(client.state());
      Serial.println(". Retrying in 2 seconds...");
      delay(2000); // Fail-safe delay to prevent aggressive connection spamming
    }
  }
}

void loop() {
  // Ensure the MQTT connection stays alive; reconnect if dropped
  if (!client.connected()) {
    reconnect();
  }

  // Allow the underlying MQTT client to handle keep-alive pings and processing incoming data
  client.loop();

  // Read environment metrics from the DHT11 sensor
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // Verify that the sensor readings are valid numbers before constructing payload
  if (isnan(temp) || isnan(hum)) {
    Serial.println("Error: Failed to read data from the DHT11 sensor!");
    delay(2000);
    return;
  }

  // Construct a standard JSON payload format required by ThingsBoard
  // Expected structure: {"temperature":XX.XX,"humidity":YY.YY}
  String payload = "{";
  payload += "\"temperature\":";
  payload += temp;
  payload += ",";
  payload += "\"humidity\":";
  payload += hum;
  payload += "}";

  // Publish the raw JSON payload to the default ThingsBoard telemetry endpoint
  client.publish(telemetry_topic, payload.c_str());

  // Output local debug data to the Serial Monitor
  Serial.println("Telemetry successfully transmitted to ThingsBoard:");
  Serial.println(payload);

  // Wait 5 seconds before repeating the loop cycle
  delay(5000);
}