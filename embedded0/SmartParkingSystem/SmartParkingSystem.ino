#include <Servo.h>

// Pin Definitions
#define TRIG_PIN 9
#define ECHO_PIN 8

#define SERVO_PIN A1

#define IR1_PIN 2
#define IR2_PIN 3
#define IR3_PIN 4

#define LED_SLOT1 13
#define LED_SLOT2 6
#define LED_SLOT3 7

#define GATE_LED 11

#define BUZZER_PIN 12

// Global Variables
Servo gateServo;

const int gateOpenAngle = 90;  // Angle to open gate
const int gateCloseAngle = 0;  // Angle to close gate

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(IR1_PIN, INPUT_PULLUP);
  pinMode(IR2_PIN, INPUT_PULLUP);
  pinMode(IR3_PIN, INPUT_PULLUP);

  pinMode(LED_SLOT1, OUTPUT);
  pinMode(LED_SLOT2, OUTPUT);
  pinMode(LED_SLOT3, OUTPUT);

  pinMode(GATE_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  gateServo.attach(SERVO_PIN);
  gateServo.write(gateCloseAngle); // Start with gate closed

  digitalWrite(LED_SLOT1, LOW);
  digitalWrite(LED_SLOT2, LOW);
  digitalWrite(LED_SLOT3, LOW);
  digitalWrite(GATE_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}

void loop() {
  // Measure distance using ultrasonic sensor
  long duration = getUltrasonicDistance();
  float distance_cm = duration * 0.034 / 2;

  // Handle invalid distance readings
  if (distance_cm < 2 || distance_cm > 400) {
    distance_cm = 400; // No detection / out of range
  }

  // Read IR sensors (LOW = occupied, HIGH = free)
  bool slot1Occupied = digitalRead(IR1_PIN) == LOW;
  bool slot2Occupied = digitalRead(IR2_PIN) == LOW;
  bool slot3Occupied = digitalRead(IR3_PIN) == LOW;

  // Control slot LEDs (RED ON if occupied)
  digitalWrite(LED_SLOT1, slot1Occupied ? HIGH : LOW);
  digitalWrite(LED_SLOT2, slot2Occupied ? HIGH : LOW);
  digitalWrite(LED_SLOT3, slot3Occupied ? HIGH : LOW);

  // Count available slots
  int availableSlots = 3 - (slot1Occupied + slot2Occupied + slot3Occupied);

  // Display current status on Serial Monitor
  Serial.print("Duration: ");
  Serial.print(duration);
  Serial.print(" us | Distance: ");
  Serial.print(distance_cm, 1);
  Serial.print(" cm | Slots available: ");
  Serial.println(availableSlots);

  // If vehicle detected at entrance (≤ 20 cm) and slots available, open gate
  if (distance_cm <= 20 && availableSlots > 0) {
    openGate();
    delay(3000); // Vehicle enters
    closeGate();
  }

  // Activate buzzer and gate LED alert only if
  // all slots are occupied AND vehicle detected within 20 cm
  if (slot1Occupied && slot2Occupied && slot3Occupied && distance_cm <= 20) {
    alertNoSlot();
  } else {
    digitalWrite(GATE_LED, LOW);  // Turn off gate LED
    noTone(BUZZER_PIN);           // Turn off buzzer
  }

  delay(500);
}

// Ultrasonic sensor function to get echo pulse duration
long getUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 40000); // Timeout 40ms (~6.8m max)

  if (duration == 0) {
    return 40000; // No pulse received
  }
  return duration;
}

// Function to open gate using servo smoothly
void openGate() {
  Serial.println("Gate opening...");
  for (int angle = gateCloseAngle; angle <= gateOpenAngle; angle += 2) {
    gateServo.write(angle);
    delay(15);
  }
}

// Function to close gate using servo smoothly
void closeGate() {
  Serial.println("Gate closing...");
  for (int angle = gateOpenAngle; angle >= gateCloseAngle; angle -= 2) {
    gateServo.write(angle);
    delay(15);
  }
}

// Function to alert no slots available with buzzer and gate LED blinking
void alertNoSlot() {
  static unsigned long lastToggle = 0;
  static bool ledState = false;
  unsigned long currentMillis = millis();

  if (currentMillis - lastToggle >= 500) {
    ledState = !ledState;
    lastToggle = currentMillis;

    digitalWrite(GATE_LED, ledState ? HIGH : HIGH);
    if (ledState) {
      tone(BUZZER_PIN, 1000); // 1 kHz tone ON
    } else {
      noTone(BUZZER_PIN);     // OFF
    }
  }
}
