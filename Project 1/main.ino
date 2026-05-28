// Hardware Pin Definitions
#define LDR_PIN A0   // Analog input pin connected to the LDR sensor
#define LED_PIN D1   // Digital output pin connected to the indicator LED

// Darkness Threshold (0 to 1023): Higher means darker ambient environment required to trigger LED
int threshold = 700;

void setup() {
  // Initialize hardware serial baud rate for local console printouts
  Serial.begin(115200);

  // Configure digital pin directional states
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // Read raw voltage from LDR sensor on the built-in Analog-to-Digital Converter (ADC)
  int ldrValue = analogRead(LDR_PIN);

  Serial.print("LDR Value: ");
  Serial.println(ldrValue);

  // --- CONDITION 1: ENVIRONMENT IS DARK ---
  if (ldrValue > threshold) {
    // Turn local hardware LED on
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Status: LED ON");
  }
  // --- CONDITION 2: ENVIRONMENT IS BRIGHT ---
  else {
    // Turn local hardware LED off
    digitalWrite(LED_PIN, LOW);
    Serial.println("Status: LED OFF");
  }

  // Enforce a structured polling rate delay of 1 second per cycle
  delay(1000);
}