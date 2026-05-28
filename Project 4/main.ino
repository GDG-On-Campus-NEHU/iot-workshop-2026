// Pin Definitions
const int trigPin = 5;  // D1 on NodeMCU
const int echoPin = 4;  // D2 on NodeMCU
const int greenLed = 14; // D5 on NodeMCU
const int redLed = 12;  // D6 on NodeMCU

// Parking Threshold (in centimeters)
// Change this value to adjust the parking sensitivity
const int PARKING_THRESHOLD = 20;

// Variables
long duration;
float distanceCm;

void setup() {
  // Initialize Serial Monitor for debugging
  Serial.begin(115200);
  Serial.println("\n--- Local Ultrasonic Parking Assistant ---");

  // Define pin modes
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(redLed, OUTPUT);

  // Flash LEDs once at startup to verify they work
  digitalWrite(greenLed, HIGH);
  digitalWrite(redLed, HIGH);
  delay(500);
  digitalWrite(greenLed, LOW);
  digitalWrite(redLed, LOW);
}

void loop() {
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
    // Blink both LEDs rapidly to indicate an error
    digitalWrite(greenLed, HIGH);
    digitalWrite(redLed, HIGH);
    delay(100);
    digitalWrite(greenLed, LOW);
    digitalWrite(redLed, LOW);
    delay(100);
    return;
  }

  // 4. Print results to Serial Monitor
  Serial.print("Distance: ");
  Serial.print(distanceCm);
  Serial.print(" cm  |  Status: ");

  // 5. LED Control Logic
  if (distanceCm < PARKING_THRESHOLD) {
    // Something is too close! Turn on Red, turn off Green
    digitalWrite(redLed, HIGH);
    digitalWrite(greenLed, LOW);
    Serial.println("[ OCCUPIED / TOO CLOSE ]");
  } else {
    // Space is clear! Turn on Green, turn off Red
    digitalWrite(greenLed, HIGH);
    digitalWrite(redLed, LOW);
    Serial.println("[ AVAILABLE ]");
  }

  // Small delay before taking the next measurement
  delay(200);
}