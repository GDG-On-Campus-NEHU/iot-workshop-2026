// WiFi Configuration Settings
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ThingsBoard Setup
const char* mqtt_server = "mqtt.thingsboard.cloud"; // Using global cloud endpoint
const char* token = "YOUR_THINGSBOARD_TOKEN";

#define RELAY_PIN D1

WiFiClient espClient;
PubSubClient client(espClient);

// Tracks the current state of the relay for telemetry reporting
bool relayState = false;

unsigned long lastTelemetryTime = 0;
const unsigned long telemetryInterval = 3000; // Report status every 3 seconds

void setup() {
  Serial.begin(115200);

  // Set relay pin as output and turn it OFF immediately on startup
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Active-Low: HIGH means OFF

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
}

/**
 * Intercepts incoming RPC commands from the ThingsBoard Dashboard
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("\n[RPC Command Received] Topic: ");
  Serial.println(topic);

  // Convert payload byte array to String
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Payload: ");
  Serial.println(message);

  // Parse incoming JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.print("JSON Parse Failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* method = doc["method"];

  if (method != NULL && strcmp(method, "setPumpState") == 0) {
    // Get the boolean value sent by the dashboard switch (true/false)
    bool targetState = doc["params"].as<bool>();

    if (targetState == true) {
      // Turn relay ON (Active-Low needs LOW voltage)
      digitalWrite(RELAY_PIN, LOW);
      relayState = true;
      Serial.println(">> RELAY TRIGGERED: ON <<");
    } else {
      // Turn relay OFF (Active-Low needs HIGH voltage)
      digitalWrite(RELAY_PIN, HIGH);
      relayState = false;
      Serial.println(">> RELAY TRIGGERED: OFF <<");
    }

    // Send acknowledgement back to ThingsBoard to confirm execution
    String topicStr = String(topic);
    if (topicStr.startsWith("v1/devices/me/rpc/request/")) {
      String requestId = topicStr.substring(26);
      String responseTopic = "v1/devices/me/rpc/response/" + requestId;

      String responsePayload = "{\"relayState\":" + String(relayState ? "true" : "false") + "}";
      client.publish(responseTopic.c_str(), responsePayload.c_str());
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to ThingsBoard Server...");
    if (client.connect(token, token, NULL)) {
      Serial.println(" Connected!");

      // Subscribe to server-side RPC command topic
      client.subscribe("v1/devices/me/rpc/request/+");
      Serial.println("Subscribed to RPC Channel.");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" -> Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // Constantly report back the current relay state to the dashboard
  unsigned long currentMillis = millis();
  if (currentMillis - lastTelemetryTime >= telemetryInterval) {
    lastTelemetryTime = currentMillis;

    String payload = "{\"pumpStatus\":" + String(relayState ? "true" : "false") + "}";
    client.publish("v1/devices/me/telemetry", payload.c_str());
    Serial.print("Sent Status Telemetry: ");
    Serial.println(payload);
  }
}