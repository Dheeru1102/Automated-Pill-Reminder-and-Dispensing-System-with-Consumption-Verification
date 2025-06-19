#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <Keypad.h>

#define TRIG_PIN 10
#define ECHO_PIN 11
#define IR_SENSOR_PIN 2
#define BUZZER_PIN 9

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myServo;

bool medicineTakenFlag = false;
int currentServoAngle = 0;
int doseCounter = 0;
String enteredPassword = "";
String correctPassword = "258";

unsigned long previousMillis = 0;
int seconds = 0, minutes = 31, hours = 11;
int days = 29, months = 4, years = 2025;

const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {8, 7, 6, 5};
byte colPins[COLS] = {1, 4, 3};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

struct Reminder {
  int day;
  int hour;
  int minute;
  int angle;
  const char *message;
};

Reminder reminders[] = {
  {29, 11, 32, 30, "Morning Meds"},
  {29, 11, 33, 60, "Afternoon Meds"},
  {29, 11, 34, 90, "Night Meds"},
  {29, 11, 35, 120, "Morning Meds"},
  {29, 11, 36, 160, "Afternoon Meds"},
  {29, 11, 37, 180, "Night Meds"}
};

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.begin(16, 2);
  lcd.backlight();
  myServo.attach(12);
  myServo.write(0);
  currentServoAngle = 0;

  Serial.begin(9600);
  Serial.println("Medicine Dispenser Initialized...");
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 1000) {
    previousMillis = currentMillis;
    seconds++;
    if (seconds == 60) { seconds = 0; minutes++; }
    if (minutes == 60) { minutes = 0; hours++; }
    if (hours == 24) { hours = 0; days++; }
    if (days > 30) { days = 1; months++; }
    if (months > 12) { months = 1; years++; }

    displayDateTime();
  }

  if (doseCounter < 6) {
    checkReminders();
  }
}

void displayDateTime() {
  lcd.setCursor(0, 0);
  lcd.print("Date: "); lcd.print(days); lcd.print("/"); lcd.print(months); lcd.print("/"); lcd.print(years);
  lcd.setCursor(0, 1);
  lcd.print("Time: ");
  if (hours < 10) lcd.print("0"); lcd.print(hours);
  lcd.print(":");
  if (minutes < 10) lcd.print("0"); lcd.print(minutes);
  lcd.print(":");
  if (seconds < 10) lcd.print("0"); lcd.print(seconds);

  // Serial Monitor Display
  Serial.print("Date: "); Serial.print(days); Serial.print("/");
  Serial.print(months); Serial.print("/"); Serial.print(years);
  Serial.print(" Time: ");
  Serial.print(hours); Serial.print(":");
  Serial.print(minutes); Serial.print(":");
  Serial.println(seconds);
}

void checkReminders() {
  for (int i = 0; i < sizeof(reminders) / sizeof(Reminder); i++) {
    if (days == reminders[i].day && hours == reminders[i].hour && minutes == reminders[i].minute && seconds == 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(reminders[i].message);
      Serial.println(reminders[i].message);
      triggerBuzzer(reminders[i].angle);
    }
  }
}

void triggerBuzzer(int angle) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Password");
  Serial.println("Buzzer ON - Enter Password");

  // Continuously ring the buzzer until password is correct
  while (true) {
    tone(BUZZER_PIN, 1000);
    enterPassword();
    if (enteredPassword == correctPassword) {
      noTone(BUZZER_PIN);
      Serial.println("Password Correct - Buzzer OFF");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Password Correct");
      delay(1000);
      waitForIRSensor(angle);
      break;
    } else {
      Serial.println("Wrong Password Entered!");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wrong Password");
      delay(1000);
    }
  }
}

void enterPassword() {
  enteredPassword = "";
  lcd.setCursor(0, 0);
  lcd.print("Enter Password:");

  while (enteredPassword.length() < 3) {
    char key = keypad.getKey();
    if (key) {
      lcd.setCursor(enteredPassword.length(), 1);
      lcd.print('*');
      enteredPassword += key;
    }
  }
  Serial.print("Password Entered: ");
  Serial.println(enteredPassword);
}

void waitForIRSensor(int angle) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place Hand...");

  while (digitalRead(IR_SENSOR_PIN) == HIGH);
  delay(500);
  rotateServo(angle);
  waitForMedicine();
}

void rotateServo(int angle) {
  if (angle == currentServoAngle) return;
  for (int pos = currentServoAngle; pos <= angle; pos++) {
    myServo.write(pos);
    delay(15);
  }
  currentServoAngle = angle;
  Serial.print("Servo Rotated to: ");
  Serial.print(angle);
  Serial.println(" degrees.");
}

void waitForMedicine() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Take Medicine...");

  unsigned long startTime = millis();
  medicineTakenFlag = false;

  while (!medicineTakenFlag && millis() - startTime < 60000) {
    float distance = getUltrasonicDistance();
    Serial.print("Ultrasonic Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    if (distance >= 1 && distance <= 5) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Medicine Taken");
      Serial.println("✅ Medicine Taken Successfully");
      medicineTakenFlag = true;
      doseCounter++;
      break;
    }
  }

  if (!medicineTakenFlag) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Missed Dose");
    Serial.println("❌ Missed Dose!");
  }
  delay(2000);
  lcd.clear();
}

float getUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}
