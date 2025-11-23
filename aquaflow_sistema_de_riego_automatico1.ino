#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Botones
#define BTN_NEXT 2
#define BTN_SELECT 3
#define BTN_BACK 4

// Sensores
int soilPin = A0;
int tempPin = A1;

// L293D Motor Pins
int ENA = 5;   // PWM
int IN1 = 8;
int IN2 = 12;

// LEDs
int ledMotor = 6;
int rgbRed = 11;
int rgbBlue = 10;
int rgbGreen = 9;

// Menu
int menuState = 0;
int selectedPlant = 0;

// Plantas
String plants[] = {
  "Cactus", "Ruda", "Lavanda",
  "Romero", "Menta", "Albahaca",
  "Pasto"
};

int plantHumidity[] = {25, 50, 45, 45, 65, 75, 70};
int totalPlants = 7;

// Anti-flicker
String lastLine0 = "";
String lastLine1 = "";

// ---------- PID VARIABLES ----------
float Kp = 4.0;
float Ki = 0.4;
float Kd = 1.8;

float integral = 0;
float lastError = 0;
unsigned long lastTime = 0;

// -----------------------------------------
void updateLCD(int row, String text) {
  text = text.substring(0, 16);
  if (row == 0 && text == lastLine0) return;
  if (row == 1 && text == lastLine1) return;

  lcd.setCursor(0, row);
  lcd.print("                ");
  lcd.setCursor(0, row);
  lcd.print(text);

  if (row == 0) lastLine0 = text;
  else lastLine1 = text;
}

// -----------------------------------------
bool buttonPressed(int pin) {
  if (digitalRead(pin) == HIGH) {
    delay(40);
    return digitalRead(pin) == HIGH;
  }
  return false;
}

// -----------------------------------------
int readMoisture() {
  int raw = analogRead(soilPin);
  int humidity = map(raw, 0, 876, 0, 100);
  return constrain(humidity, 0, 100);
}

// -----------------------------------------
float readTemperatureC() {
  int raw = analogRead(tempPin);
  float voltage = raw * (5.0 / 1023.0);
  return (voltage - 0.5) * 100.0;
}

// -----------------------------------------
int computePID(int target, int current) {

  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0;

  float error = target - current;

  // termino integral
  integral += error * dt;

  // derivada integral
  float derivative = (error - lastError) / dt;

  // PID saliad
  float output = Kp * error + Ki * integral + Kd * derivative;

  // guardado de datos
  lastError = error;
  lastTime = now;

  // PWM rango
  return constrain((int)output, 0, 255);
}

// -----------------------------------------
void updateRGB(int humidity, int target) {

  if (humidity < target - 10) {
    digitalWrite(rgbRed, HIGH);
    digitalWrite(rgbBlue, LOW);
    digitalWrite(rgbGreen, LOW);
  }
  else if (humidity < target) {
    digitalWrite(rgbRed, LOW);
    digitalWrite(rgbBlue, HIGH);
    digitalWrite(rgbGreen, LOW);
  }
  else {
    digitalWrite(rgbRed, LOW);
    digitalWrite(rgbBlue, LOW);
    digitalWrite(rgbGreen, HIGH);
  }
}

// -----------------------------------------
void motorControlPID(int target, int humidity) {

  int speed = computePID(target, humidity);

  // Cuando ya se supera la meta → apagar motor
  if (humidity >= target) {
    analogWrite(ENA, 0);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(ledMotor, LOW);
    return;
  }

  // Motor adelante
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Aplicar PWM
  analogWrite(ENA, speed);

  // LED motor ON si PWM > 0
  digitalWrite(ledMotor, speed > 5 ? HIGH : LOW);
}

// -----------------------------------------
void setup() {
  pinMode(BTN_NEXT, INPUT);
  pinMode(BTN_SELECT, INPUT);
  pinMode(BTN_BACK, INPUT);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ledMotor, OUTPUT);

  pinMode(rgbRed, OUTPUT);
  pinMode(rgbBlue, OUTPUT);
  pinMode(rgbGreen, OUTPUT);

  lcd.init();
  lcd.backlight();

  updateLCD(0, "   AquaFlow PID");
  updateLCD(1, "   Iniciando...");
  delay(1200);

  lastTime = millis();
}

// -----------------------------------------
void loop() {

  int humidity = readMoisture();
  float tempC = readTemperatureC();

  if (menuState == 0) {

    updateLCD(0, "Elige planta:");
    updateLCD(1, plants[selectedPlant]);

    if (buttonPressed(BTN_NEXT)) {
      selectedPlant++;
      if (selectedPlant >= totalPlants) selectedPlant = 0;
    }

    if (buttonPressed(BTN_SELECT)) {
      menuState = 1;
      integral = 0; // Reset PID
      lastError = 0;
    }

    return;
  }

  if (menuState == 1) {

    int target = plantHumidity[selectedPlant];

    // PID Motor Control
    motorControlPID(target, humidity);

    // RGB Indicator
    updateRGB(humidity, target);

    // LCD display
    updateLCD(0, plants[selectedPlant] + " T:" + String((int)tempC) + "C");
    updateLCD(1, "H:" + String(humidity) + "% Meta:" + String(target) + "%");

    if (buttonPressed(BTN_BACK)) {
      menuState = 0;
    }
  }
}




