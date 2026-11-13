#include <LiquidCrystal.h>
#include <Servo.h>
#include <EEPROM.h> // added

/* ---------- LCD ---------- */
LiquidCrystal lcd(A4, A5, A3, A2, A1, A0);

/* ---------- SERVO ---------- */
Servo lockServo;

/* ---------- Keypad Pins ---------- */
const byte rowPins[4] = {9, 8, 7, 6};
const byte colPins[3] = {12, 11, 10};

char keys[4][3] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

/* ---------- Admin Password ---------- */
const String ADMIN_PASSWORD = "242625";

/* ---------- User Database ---------- */
const int maxUsers = 5;
String userFP[maxUsers];
String userPass[maxUsers];
String userCode[maxUsers];
int userCount = 0;

/* ---------- State Machine ---------- */
enum State {
  MENU,
  FINGERPRINT_LOGIN,
  VERIFY_PASS,
  ADMIN_MENU,
  REGISTER_FP,
  REGISTER_PASS,
  DELETE_FP,          // kept but NOT used
  ADMIN_VERIFY_PASS,
  DELETE_BY_ID        // NEW
};

State currentState = MENU;

String inputBuffer = "";
int matchedIndex = -1;

/* ---------- EEPROM Helpers ---------- */
void saveToEEPROM() {
  EEPROM.put(0, userCount);
  for (int i = 0; i < maxUsers; i++) {
    char fp[50]; userFP[i].toCharArray(fp, 50); EEPROM.put(10 + i*50, fp);
    char pass[20]; userPass[i].toCharArray(pass, 20); EEPROM.put(260 + i*20, pass);
    char code[10]; userCode[i].toCharArray(code, 10); EEPROM.put(360 + i*10, code);
  }
}

void loadFromEEPROM() {
  EEPROM.get(0, userCount);
  if (userCount < 0 || userCount > maxUsers) userCount = 0;
  for (int i = 0; i < userCount; i++) {
    char fp[50]; EEPROM.get(10 + i*50, fp); userFP[i] = String(fp);
    char pass[20]; EEPROM.get(260 + i*20, pass); userPass[i] = String(pass);
    char code[10]; EEPROM.get(360 + i*10, code); userCode[i] = String(code);
  }
}

/* ---------- Helpers ---------- */
String readFingerprintLine() {
  String fp = Serial.readStringUntil('\n');
  if (fp.length() == 0) fp = Serial.readStringUntil('\r');
  fp.trim();
  return fp;
}

String formatID(int num) {
  char buf[5];
  sprintf(buf, "%04d", num);
  return String(buf);
}

void showHome() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("1.Verify 2.Admin");
  lcd.setCursor(0,1);
  lcd.print("*=Back #=Next");
}

/* ---------- Setup ---------- */
void setup() {
  lcd.begin(16, 2);
  lockServo.attach(5);
  lockServo.write(0);

  Serial.begin(9600);

  for (int i = 0; i < 4; i++) pinMode(rowPins[i], INPUT_PULLUP);
  for (int j = 0; j < 3; j++) {
    pinMode(colPins[j], OUTPUT);
    digitalWrite(colPins[j], HIGH);
  }

  loadFromEEPROM(); // added
  resetToMenu();
}

/* ---------- Loop ---------- */
void loop() {

  if (Serial.available()) {
    String fp = readFingerprintLine();

    switch (currentState) {

      case MENU:
      case FINGERPRINT_LOGIN:
        matchedIndex = -1;
        for (int i = 0; i < userCount; i++) {
          if (userFP[i] == fp) {
            matchedIndex = i;
            break;
          }
        }

        if (matchedIndex != -1) {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Enter Password:");
          inputBuffer = "";
          currentState = VERIFY_PASS;
        } else {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("FP Not Found");
          delay(1500);
          resetToMenu();
        }
        break;

      case REGISTER_FP:
        if (userCount < maxUsers) {
          userFP[userCount] = fp;
          userCode[userCount] = formatID(userCount + 1);
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Set Password:");
          inputBuffer = "";
          currentState = REGISTER_PASS;
        } else {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("DB Full");
          delay(1500);
          resetToMenu();
        }
        break;

      default:
        break;
    }
  }

  char key = scanKeypad();
  if (key != 0) handleKey(key);
}

/* ---------- Keypad Scanner ---------- */
char scanKeypad() {
  for (int col = 0; col < 3; col++) {
    digitalWrite(colPins[col], LOW);
    for (int row = 0; row < 4; row++) {
      if (digitalRead(rowPins[row]) == LOW) {
        digitalWrite(colPins[col], HIGH);
        delay(250);
        return keys[row][col];
      }
    }
    digitalWrite(colPins[col], HIGH);
  }
  return 0;
}

/* ---------- Key Handler ---------- */
void handleKey(char key) {

  switch (currentState) {

    case MENU:
      if (key == '1') {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Give Fingerprint");
        currentState = FINGERPRINT_LOGIN;
      } 
      else if (key == '2') {
        currentState = ADMIN_MENU;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("1.Reg 2.Del");
      }
      break;

    case VERIFY_PASS:
      if (key == '*') {
        if (inputBuffer.length() > 0)
          inputBuffer.remove(inputBuffer.length()-1);
      }
      else if (key == '#') {
        if (userPass[matchedIndex] == inputBuffer) {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Welcome");
          lcd.setCursor(0,1);
          lcd.print(userCode[matchedIndex]);
          lockServo.write(90);
          delay(5000);
          lockServo.write(0);
        } else {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Wrong Password");
          delay(1500);
        }
        resetToMenu();
      }
      else inputBuffer += key;
      break;

    case ADMIN_MENU:
      if (key == '1') {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Give Fingerprint");
        currentState = REGISTER_FP;
      }
      else if (key == '2') {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Admin Password:");
        inputBuffer = "";
        currentState = ADMIN_VERIFY_PASS;
      }
      else if (key == '*') resetToMenu();
      break;

    case ADMIN_VERIFY_PASS:
      if (key == '*') {
        if (inputBuffer.length() > 0)
          inputBuffer.remove(inputBuffer.length()-1);
      }
      else if (key == '#') {
        if (inputBuffer == ADMIN_PASSWORD) {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Enter User ID:");
          inputBuffer = "";
          currentState = DELETE_BY_ID;
        } else {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Wrong Password");
          delay(1500);
          resetToMenu();
        }
      }
      else inputBuffer += key;
      break;

    case DELETE_BY_ID:
      if (key == '*') {
        if (inputBuffer.length() > 0)
          inputBuffer.remove(inputBuffer.length()-1);
      }
      else if (key == '#') {
        bool found = false;
        for (int i = 0; i < userCount; i++) {
          if (userCode[i] == inputBuffer) {
            for (int j = i; j < userCount - 1; j++) {
              userFP[j]   = userFP[j+1];
              userPass[j] = userPass[j+1];
              userCode[j] = userCode[j+1];
            }
            userCount--;
            saveToEEPROM(); // added
            found = true;
            break;
          }
        }
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(found ? "User Deleted" : "ID Not Found");
        delay(1500);
        resetToMenu();
      }
      else inputBuffer += key;
      break;

    case REGISTER_PASS:
      if (key == '*') {
        if (inputBuffer.length() > 0)
          inputBuffer.remove(inputBuffer.length()-1);
      }
      else if (key == '#') {
        if (inputBuffer.length() > 0) {
          userPass[userCount] = inputBuffer;
          userCount++;
          saveToEEPROM(); // added
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Registered ID");
          lcd.setCursor(0,1);
          lcd.print(userCode[userCount-1]);
          delay(2000);
        }
        resetToMenu();
      }
      else inputBuffer += key;
      break;

    default:
      break;
  }

  lcd.setCursor(0,1);
  lcd.print("                ");
  lcd.setCursor(0,1);
  for (int i = 0; i < inputBuffer.length(); i++) lcd.print('*');
}

/* ---------- Reset ---------- */
void resetToMenu() {
  currentState = MENU;
  inputBuffer = "";
  matchedIndex = -1;
  showHome();
}
