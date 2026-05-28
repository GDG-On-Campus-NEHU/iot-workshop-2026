#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// WiFi Configuration Settings
const char* ssid = "YOUR_WIFI_SSID";          // SENSITIVE INFO REMOVED: Place your Wi-Fi SSID here
const char* password = "YOUR_WIFI_PASSWORD";  // SENSITIVE INFO REMOVED: Place your Wi-Fi Password here

// ThingsBoard Cloud MQTT Broker Configuration
const char* mqtt_server = "mqtt.eu.thingsboard.cloud";
const char* token = "YOUR_THINGSBOARD_TOKEN";  // SENSITIVE INFO REMOVED: Place your Device Access Token here

// Hardware Pin Definitions
#define SOIL_PIN A0    // Analog pin for Soil Moisture Sensor
#define RELAY_PIN D1   // Digital pin controlling the Water Pump Relay
#define DHTPIN D2      // Digital pin connected to the DHT11 Data line
#define DHTTYPE DHT11  // Specifying the DHT variant (DHT11)

// Initialize DHT Sensor instance
DHT dht(DHTPIN, DHTTYPE);

// Initialize Network and MQTT Client instances
WiFiClient espClient;
PubSubClient client(espClient);

// Calibration: Adjust based on your specific sensor baseline (Dry vs Wet analog bounds)
int soilThreshold = 600;

void setup() {
  // Initialize hardware serial communication for local diagnostics
  Serial.begin(115200);

  // Configure Relay Pin as output and turn it off immediately (Active Low Configuration)
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Logic HIGH breaks circuit = Relay OFF

  // Initialize the DHT sensor array
  dht.begin();

  // Initiate Wi-Fi Connection sequence
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connected!");
  Serial.print("Local IP Address: ");
  Serial.println(WiFi.localIP());

  // Configure the MQTT Client properties
  client.setServer(mqtt_server, 1883);
}

void reconnect() {
  // Loop until a stable MQTT session is re-established
  while (!client.connected()) {
    Serial.print("Attempting ThingsBoard MQTT connection...");

    // Connect using the token as both ClientID and Username (ThingsBoard standard specification)
    if (client.connect(token, token, NULL)) {
      Serial.println("Connected to ThingsBoard!");
    } else {
      Serial.print("Failed connection, rc=");
      Serial.print(client.state());
      Serial.println(" -> Retrying in 2 seconds...");
      delay(2000);
    }
  }
}

void loop() {
  // Guard clause ensuring network telemetry persistence
  if (!client.connected()) reconnect();
  client.loop();

  // Read raw ambient data from soil and environmental sensor units
  int soilValue = analogRead(SOIL_PIN);
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // Validate integrity of sensor data packets
  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT Sensor Read Error: Check hardware connections.");
    return;
  }

  // Automatic Local Irrigation Control Logic
  bool pumpState;
  if (soilValue > soilThreshold) {
    // Value goes up when soil gets dry -> Turn pump ON (Active Low)
    digitalWrite(RELAY_PIN, LOW);
    pumpState = true;
  } else {
    // Soil moisture content is acceptable -> Keep pump OFF
    digitalWrite(RELAY_PIN, HIGH);
    pumpState = false;
  }

  // Serialize telemetry properties into raw JSON payload format
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

  // Publish telemetry packet string to the required ThingsBoard device topic path
  client.publish("v1/devices/me/telemetry", payload.c_str());

  // Print mirrored payload output string to local console line
  Serial.println(payload);

  // Polling rate pacing interval (5-second delay cycles)
  delay(5000);
}