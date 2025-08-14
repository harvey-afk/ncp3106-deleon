#include <DHT.h>
#include <Servo.h>

// Sensor and Actuator Pins
#define DHTPIN 10
#define DHTTYPE DHT22
#define RAIN_SENSOR_PIN A0
#define WATER_LEVEL_PIN A1
#define BUZZER_PIN 11
#define SERVO_PIN 12

// Temperature LEDs
const int tempLowLED = 2;    // Green
const int tempMedLED = 3;    // Yellow
const int tempHighLED = 4;   // Red

// Water Level LEDs
const int level1LED = 5;     // Green
const int level2LED = 6;     // Yellow
const int level3and4LED = 7; // Red (blinks for Level 4)

// Humidity LEDs
const int humidifierLED = A2;     // Yellow - too dry (<40%)
const int dehumidifierLED = A3;   // Red - too humid (>60%)

DHT dht(DHTPIN, DHTTYPE);
Servo servo;

unsigned long previousMillis = 0;
const long interval = 2000;  // sensor read interval in ms

unsigned long blinkPreviousMillis = 0;
const long blinkInterval = 500;  // blinking interval for water level alert

bool blinkState = false;

void setup() {
  Serial.begin(9600);
  dht.begin();
  servo.attach(SERVO_PIN);

  pinMode(tempLowLED, OUTPUT);
  pinMode(tempMedLED, OUTPUT);
  pinMode(tempHighLED, OUTPUT);

  pinMode(level1LED, OUTPUT);
  pinMode(level2LED, OUTPUT);
  pinMode(level3and4LED, OUTPUT);

  pinMode(humidifierLED, OUTPUT);
  pinMode(dehumidifierLED, OUTPUT);

  pinMode(BUZZER_PIN, OUTPUT);

  servo.write(0);  // Start servo at 0 degrees (open)
}

// Retry reading DHT sensor up to 5 times
bool readDHT(float &temp, float &hum) {
  for (int i = 0; i < 5; i++) {
    temp = dht.readTemperature();
    hum = dht.readHumidity();
    if (!isnan(temp) && !isnan(hum)) return true;
    delay(200);
  }
  return false;
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float temp, hum;
    if (!readDHT(temp, hum)) {
      Serial.println("Failed to read from DHT sensor!");
      // Turn off LEDs if sensor fails
      digitalWrite(tempLowLED, LOW);
      digitalWrite(tempMedLED, LOW);
      digitalWrite(tempHighLED, LOW);
      digitalWrite(humidifierLED, LOW);
      digitalWrite(dehumidifierLED, LOW);
      delay(500);
      return;
    }

    int rainVal = analogRead(RAIN_SENSOR_PIN);
    int waterLevelVal = analogRead(WATER_LEVEL_PIN);

    Serial.print("Temp: "); Serial.print(temp);
    Serial.print(" °C | Humidity: "); Serial.print(hum);
    Serial.print(" % | Rain: "); Serial.print(rainVal);
    Serial.print(" | Water Level: "); Serial.println(waterLevelVal);

    // Temperature LEDs
    digitalWrite(tempLowLED, temp < 30);
    digitalWrite(tempMedLED, temp >= 30 && temp < 35);
    digitalWrite(tempHighLED, temp >= 35);

    // Humidity LEDs
    if (hum < 40) {
      digitalWrite(humidifierLED, HIGH);    // Too Dry
      digitalWrite(dehumidifierLED, LOW);
    } else if (hum > 60) {
      digitalWrite(humidifierLED, LOW);
      digitalWrite(dehumidifierLED, HIGH);  // Too Humid
    } else {
      digitalWrite(humidifierLED, LOW);
      digitalWrite(dehumidifierLED, LOW);
    }

    // Rain detection
    if (rainVal < 500) {
        servo.write(90);  // Close servo
    } else {
        servo.write(0);   // Open servo
    }

    // Water level LEDs (no buzzer here; buzzer controlled below for blinking)
    if (waterLevelVal <= 150) {
      digitalWrite(level1LED, HIGH);
      digitalWrite(level2LED, LOW);
      digitalWrite(level3and4LED, LOW);
    } else if (waterLevelVal <= 300) {
      digitalWrite(level1LED, LOW);
      digitalWrite(level2LED, HIGH);
      digitalWrite(level3and4LED, LOW);
    } else if (waterLevelVal <= 450) {
      digitalWrite(level1LED, LOW);
      digitalWrite(level2LED, LOW);
      digitalWrite(level3and4LED, HIGH);
    } else {
      // Level 4 handled with blinking below
      digitalWrite(level1LED, LOW);
      digitalWrite(level2LED, LOW);
    }
  }

  // Blinking LED and buzzer for Level 4 water level
  int waterLevelVal = analogRead(WATER_LEVEL_PIN);
  if (waterLevelVal > 450) {
    if (millis() - blinkPreviousMillis >= blinkInterval) {
      blinkPreviousMillis = millis();
      blinkState = !blinkState;

      digitalWrite(level3and4LED, blinkState ? HIGH : LOW);

      if (blinkState) {
        tone(BUZZER_PIN, 1000);
      } else {
        noTone(BUZZER_PIN);
      }
    }
  } else {
    // Ensure buzzer off when not level 4
    noTone(BUZZER_PIN);
  }
}