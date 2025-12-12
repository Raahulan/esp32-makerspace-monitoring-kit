/*
  ESP32 Makerspace Monitoring Kit
  - Ultrasonic Distance (HC-SR04)
  - Temperature & Humidity (DHT11)
  - OLED Display (SSD1306 I2C)

  Notes:
  - ESP32 is 3.3V logic.
  - HC-SR04 ECHO is typically 5V -> MUST use voltage divider/level shifting.
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// ---------------- OLED CONFIG ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------------- DHT CONFIG ----------------
#define DHTPIN   4
#define DHTTYPE  DHT11
DHT dht(DHTPIN, DHTTYPE);

// -------------- ULTRASONIC CONFIG ------------
#define TRIG_PIN 5
#define ECHO_PIN 18

// --------- DISPLAY / UPDATE SETTINGS ---------
const unsigned long SENSOR_INTERVAL_MS = 1000;   // sensor update every 1s
unsigned long lastReadMs = 0;

// Store values
float temperatureC = NAN;
float humidity = NAN;
float distanceCM = NAN;

// ---------- Helper: read distance (cm) -------
float readDistanceCM() {
  // Trigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read echo pulse width (timeout in microseconds)
  // ~30ms timeout gives max distance ~5m
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);

  if (duration == 0) {
    return NAN; // timeout / no echo
  }

  // Sound speed: 343 m/s => 0.0343 cm/us
  // distance = (duration * 0.0343) / 2
  return (duration * 0.0343f) / 2.0f;
}

// ---------- Helper: draw on OLED -------------
void drawOLED(float dcm, float tC, float h) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Title
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ESP32 Monitoring Kit");

  // Distance
  display.setCursor(0, 18);
  display.print("Distance: ");
  if (isnan(dcm)) display.println("-- cm");
  else {
    display.print(dcm, 1);
    display.println(" cm");
  }

  // Temperature
  display.setCursor(0, 34);
  display.print("Temp:     ");
  if (isnan(tC)) display.println("-- C");
  else {
    display.print(tC, 1);
    display.println(" C");
  }

  // Humidity
  display.setCursor(0, 50);
  display.print("Humidity: ");
  if (isnan(h)) display.println("-- %");
  else {
    display.print(h, 1);
    display.println(" %");
  }

  display.display();
}

void setup() {
  Serial.begin(115200);

  // GPIO setup
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Start sensors
  dht.begin();

  // I2C (default ESP32 pins: SDA 21, SCL 22)
  Wire.begin(21, 22);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED init failed. Check wiring/address (0x3C).");
    while (true) { delay(100); }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();
  delay(800);
}

void loop() {
  unsigned long now = millis();

  if (now - lastReadMs >= SENSOR_INTERVAL_MS) {
    lastReadMs = now;

    // Read ultrasonic
    distanceCM = readDistanceCM();

    // Read DHT11
    humidity = dht.readHumidity();
    temperatureC = dht.readTemperature(); // Celsius

    // Print to Serial (for debugging)
    Serial.print("Distance(cm): ");
    if (isnan(distanceCM)) Serial.print("NA"); else Serial.print(distanceCM, 1);

    Serial.print(" | Temp(C): ");
    if (isnan(temperatureC)) Serial.print("NA"); else Serial.print(temperatureC, 1);

    Serial.print(" | Hum(%): ");
    if (isnan(humidity)) Serial.println("NA"); else Serial.println(humidity, 1);

    // Update OLED
    drawOLED(distanceCM, temperatureC, humidity);
  }
}
