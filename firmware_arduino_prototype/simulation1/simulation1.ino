#include <LiquidCrystal.h>
#include <Servo.h>

/* ---------- LCD ---------- */
LiquidCrystal lcd(A4, A5, A3, A2, A1, A0);

/* ---------- SERVO ---------- */
Servo lockServo;

/* ---------- PASSWORD ---------- */
String password = "123";
String entered = "";

void setup() {

  lcd.begin(16, 2);
  lcd.print("Door Lock Sys");
  lcd.setCursor(0,1);
  lcd.print("Initializing");
  delay(2000);
  lcd.clear();

  lockServo.attach(5);
  lockServo.write(0);   // LOCKED

  /* Phone Keypad pins (PHONE keypad, not matrix) */
  pinMode(12, INPUT_PULLUP); // 1
  pinMode(11, INPUT_PULLUP); // 2
  pinMode(10, INPUT_PULLUP); // 3
  pinMode(9,  INPUT_PULLUP); // *
  pinMode(8,  INPUT_PULLUP); // #
  pinMode(7,  INPUT_PULLUP); // 0

  Serial.begin(9600);

  lcd.print("Enter Password");
}

void loop() {

  /* ---------- FINGERPRINT (Virtual Terminal) ---------- */
  if (Serial.available()) {
    char fp = Serial.read();

    if (fp == 'Y') {
      grantAccess("Fingerprint OK");
    }
    else if (fp == 'N') {
      denyAccess();
    }
  }

  /* ---------- KEYPAD ---------- */
  char key = readKeypad();

  if (key != 0) {

    if (key == '*') {           // ENTER
      if (entered == password) {
        grantAccess("Password OK");
      } else {
        denyAccess();
      }
      entered = "";
    }

    else if (key == '#') {      // CLEAR
      entered = "";
      lcd.clear();
      lcd.print("Cleared");
      delay(500);
      lcd.clear();
      lcd.print("Enter Password");
    }

    else {                      // DIGITS
      entered += key;
      lcd.setCursor(0,1);
      lcd.print(entered);
    }
  }
}

/* ---------- FUNCTIONS ---------- */

char readKeypad() {

  if (digitalRead(12) == LOW) { delay(250); return '1'; }
  if (digitalRead(11) == LOW) { delay(250); return '2'; }
  if (digitalRead(10) == LOW) { delay(250); return '3'; }
  if (digitalRead(7)  == LOW) { delay(250); return '0'; }
  if (digitalRead(9)  == LOW) { delay(250); return '*'; } // ENTER
  if (digitalRead(8)  == LOW) { delay(250); return '#'; } // CLEAR

  return 0;
}

void grantAccess(String msg) {
  lcd.clear();
  lcd.print("Access Granted");
  lcd.setCursor(0,1);
  lcd.print(msg);

  lockServo.write(90);   // UNLOCK
  delay(5000);

  lockServo.write(0);    // LOCK
  lcd.clear();
  lcd.print("Enter Password");
}

void denyAccess() {
  lcd.clear();
  lcd.print("Access Denied");
  delay(2000);
  lcd.clear();
  lcd.print("Enter Password");
}
