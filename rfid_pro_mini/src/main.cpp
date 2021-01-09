#include <Arduino.h>
#include <EEPROM.h>
#include <Keypad.h>
#include <MFRC522.h>
#include <SPI.h>

#define RST_pin A0
#define Chip_Select_pin 10
bool isSwitched = false;
unsigned int BLINK_TIME = 500;
const uint8_t relay_1 = A1;
const uint8_t Buzzer = A2;
const uint8_t redColor = A3;
const uint8_t greenColor = A4;
const uint8_t blueColor = A5;
byte RELAY_1_TIME = 3; // On time for relay in Seconds
const byte ROWS = 4;   // four rows
const byte COLS = 4;   // four columns
byte masterKey[4] = {9, 9, 9, 9};
byte rfidCardsRef[63][4];
byte lastStoredCardPosition = 0; // last stored rfid or code index
// define the symbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {{'1', '2', '3', 'A'},
                             {'4', '5', '6', 'B'},
                             {'7', '8', '9', 'C'},
                             {'*', '0', '#', 'D'}};
byte colPins[COLS] = {6, 7, 8, 9}; // connect to the row pinouts of the keypad
byte rowPins[ROWS] = {2, 3, 4, 5}; // connect to the column pinouts of the
                                   // keypad
// initialize an instance of class Keypad
Keypad customKeypad =
    Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
MFRC522 rfidReader(Chip_Select_pin, RST_pin); // initialize refidReader
//////////////////////////////////////////////////////
// Method for buzzer tone
void buzzerTone(int delayTime) {
  digitalWrite(Buzzer, HIGH);
  delay(delayTime);
  digitalWrite(Buzzer, LOW);
}
enum colors {
  red,
  blue,
  green,
  allOff,
};
/////////////////////////////////////////////////////
// Method for RGB led color Switching
void switchColor(int colorName) {
  if (colorName == red) {
    digitalWrite(redColor, LOW);
    digitalWrite(greenColor, HIGH);
    digitalWrite(blueColor, HIGH);
  }
  if (colorName == green) {
    digitalWrite(redColor, HIGH);
    digitalWrite(greenColor, LOW);
    digitalWrite(blueColor, HIGH);
  }
  if (colorName == blue) {
    digitalWrite(redColor, HIGH);
    digitalWrite(greenColor, HIGH);
    digitalWrite(blueColor, LOW);
  }
  if (colorName == allOff) {
    digitalWrite(redColor, HIGH);
    digitalWrite(greenColor, HIGH);
    digitalWrite(blueColor, HIGH);
  }
}
///////////////////////////////////////////////////////
// Method for error occurance visible indicator
void errorIndicator() {
  for (int i = 0; i < 3; i++) {
    switchColor(allOff);
    delay(150);
    switchColor(red);
    buzzerTone(150);
  }
  switchColor(blue);
}
///////////////////////////////////////////////////////
// Method for visible indication when a button is pressed
void buttonIsPressed(int activeColor) {
  switchColor(activeColor);
  buzzerTone(100);
  switchColor(allOff);
  delay(100);
}
///////////////////////////////////////////////////////
// Method when a task is completed
void done(int colorName) {
  for (int i = 0; i < 3; i++) {
    switchColor(allOff);
    delay(150);
    switchColor(colorName);
    buzzerTone(150);
  }
  delay(250);
}
///////////////////////////////////////////////////////
// Method for activating output relay
void energizeRelayOne(int time) {
  buzzerTone(250);
  digitalWrite(relay_1, HIGH);
  for (int i = 0; i < time; i++) {

    switchColor(green);
    delay(1000);
  }
  digitalWrite(relay_1, LOW);
  switchColor(allOff);
  delay(200);
}
/////////////////////////////////////////////////////
bool isMatched(byte readCard[]) {
  bool isCardMatched = false;

  for (int m = 0; m < 63; m++) {

    if (readCard[0] == rfidCardsRef[m][0] &&
        readCard[1] == rfidCardsRef[m][1] &&
        readCard[2] == rfidCardsRef[m][2] &&
        readCard[3] == rfidCardsRef[m][3]) {
      isCardMatched = true;
    }
  }
  return isCardMatched;
}
/////////////////////////////////////////////////////
bool isMasterKey() {
  bool masterKeyFlag = false;
  char keyStroke = customKeypad.getKey();
  if (keyStroke > 0) {
    if (keyStroke == '*') {
      buttonIsPressed(blue);
      byte tempPass[4];
      int i = 0;
      do {
        keyStroke = customKeypad.getKey();
        if (keyStroke > 0) {
          tempPass[i] = keyStroke - 48;
          buttonIsPressed(blue);
          if (keyStroke != '#') {
            i++;
          }
        }
      } while (keyStroke != '#');
      // compare obtained digits to master code
      if (i > 4) {
        errorIndicator();
        masterKeyFlag = false;
      } else {
        if (tempPass[0] == masterKey[0] && tempPass[1] == masterKey[1] &&
            tempPass[2] == masterKey[2] && tempPass[3] == masterKey[3]) {
          masterKeyFlag = true;
          switchColor(green);
          buzzerTone(250);
        } else {
          masterKeyFlag = false;
          errorIndicator();
        }
      }
    } else {
      byte index = 1;
      byte tempPass[4];
      tempPass[0] = keyStroke - 48;
      buttonIsPressed(red);
      do {
        keyStroke = customKeypad.getKey();
        if (keyStroke > 0) {
          tempPass[index] = keyStroke - 48;
          buttonIsPressed(red);
          if (keyStroke != '#') {
            index += 1;
          }
        }
      } while (keyStroke != '#');
      if (index != 4) {
        errorIndicator();
      } else {
        if (isMatched(tempPass)) {
          energizeRelayOne(RELAY_1_TIME);
        } else {
          errorIndicator();
        }
      }
    }
  }
  return masterKeyFlag;
}
//////////////////////////////////////////
void loadTimerValueFromEEPROM() {
  byte timerValue;
  timerValue = EEPROM.read(4);
  delay(10);
  RELAY_1_TIME = timerValue;
}
////////////////////////////////////
void setOutputRelayEnergizeTime() {
  byte rVal;
  byte val[2];
  uint8_t cc = 0;
  do {
    char keyStroke = customKeypad.getKey();
    if (keyStroke > 0 && keyStroke != '*' && keyStroke != '#') {

      rVal = keyStroke - 48;
      val[cc] = rVal;
      cc++;
      buttonIsPressed(green);
    }
  } while (cc < 2);
  rVal = val[1] + (val[0] * 10);
  EEPROM.write(4, rVal);
  delay(10);
  loadTimerValueFromEEPROM();
  done(blue);
}
//////////////////////////////////////////////
void loadRFIDCardsFromEEPROM() {
  for (int i = 1; i < 64; i++) {
    for (int n = 0; n < 4; n++) {
      byte data = EEPROM.read((4 * i) + 2 + n);
      delay(2);
      rfidCardsRef[i - 1][n] = data;
    }
  }
}

/////////////////////////////////////////
void addRFID_Cards() {
  int userCodePos = lastStoredCardPosition;
  char keyStroke;
  do {
    keyStroke = customKeypad.getKey();
    if (keyStroke != 0) {
      buzzerTone(100);
    }
    if (rfidReader.PICC_IsNewCardPresent()) {

      if (rfidReader.PICC_ReadCardSerial()) {
        loadRFIDCardsFromEEPROM();
        if (!isMatched(rfidReader.uid.uidByte)) {
          buzzerTone(100);
          switchColor(green);
          for (int i = 0; i < rfidReader.uid.size; i++) {
            uint8_t dataByte = rfidReader.uid.uidByte[i];
            EEPROM.write(userCodePos, dataByte);
            delay(100);
            userCodePos += 1;
          }
          switchColor(blue);
          rfidReader.PICC_HaltA();
        } else {
          errorIndicator();
        }
      }
    }
  } while (keyStroke != '#');
  EEPROM.write(0x05, userCodePos);
  lastStoredCardPosition = userCodePos;
  delay(10);
  loadRFIDCardsFromEEPROM();
  done(blue);
}
////////////////////////////////////////////////
void clearEEPROM() {
  for (int i = 6; i < 255; i++) {
    EEPROM.write(i, 0xff);
    delay(10);
  }
  EEPROM.write(5, 6);
  delay(10);
  EEPROM.write(4, 3);
  delay(10);
  loadRFIDCardsFromEEPROM();
  done(green);
  return;
}
//////////////////////////////////////////////////
void changeMasterKey() {
  byte tempPass[4];
  byte confirmationArray[4];
  int index = 0;
  char b;
  do {
    b = customKeypad.getKey();
    if (b > 0 && b != '#') {
      buttonIsPressed(blue);
      tempPass[index] = b - 48;
      if (b != '#') {
        index += 1;
      }
    }
  } while (b != '#');
  buzzerTone(100);
  index = 0;
  char keyStroke;
  do {
    keyStroke = customKeypad.getKey();
    if (keyStroke > 0 && keyStroke != '#') {
      buttonIsPressed(blue);
      confirmationArray[index] = keyStroke - 48;
      if (keyStroke != '#') {
        index += 1;
      }
    }
  } while (keyStroke != '#');
  buzzerTone(100);
  if (index != 4) {
    errorIndicator();
    return;
  }
  if (tempPass[0] == confirmationArray[0] &&
      tempPass[1] == confirmationArray[1] &&
      tempPass[2] == confirmationArray[2] &&
      tempPass[3] == confirmationArray[3]) {
    done(green);
    switchColor(blue);
    for (int y = 0; y < 4; y++) {
      masterKey[y] = tempPass[y];
    }
    for (int y = 0; y < 4; y++) {
      EEPROM.write(y, masterKey[y]);
    }
  } else {
    errorIndicator();
    return;
  }
}
///////////////////////////////////////
void loadMasterKey() {
  for (int y = 0; y < 4; y++) {
    masterKey[y] = EEPROM.read(y);
  }
}
///////////////////////////////////////
bool eepromIsAtInitial() {
  bool isIntial = false;
  byte counter = 0;
  for (int i = 0; i < 4; i++) {
    byte data = EEPROM.read(i);
    if (data == 0xFF) {
      counter += 1;
    }
  }
  if (counter != 0) {
    isIntial = true;
  }
  return isIntial;
}
//////////////////////////////////////////////////////////
void addAccessPassword() {
  byte tempPass[4];
  byte index = 0;
  char keyStroke;
  do {
    keyStroke = customKeypad.getKey();
    if (keyStroke > 0) {
      tempPass[index] = keyStroke - 48;
      buttonIsPressed(blue);
      if (keyStroke != '#') {
        index += 1;
        delay(20);
      }
    }

  } while (keyStroke != '#');
  if (index != 4) {
    errorIndicator();
  } else {
    for (int i = 0; i < index; i++) {
      EEPROM.write(lastStoredCardPosition + i, tempPass[i]);
      delay(10);
      buttonIsPressed(green);
    }
    lastStoredCardPosition = lastStoredCardPosition + index;
    EEPROM.write(0x05, lastStoredCardPosition + index);
    delay(10);
    loadRFIDCardsFromEEPROM();
    delay(10);
    done(blue);
  }
}
////////////////////////////////////////////
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(10);
  SPI.begin();
  delay(10);
  EEPROM.begin();
  delay(10);
  rfidReader.PCD_Init();
  delay(10);
  pinMode(relay_1, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(redColor, OUTPUT);
  pinMode(greenColor, OUTPUT);
  pinMode(blueColor, OUTPUT);
  switchColor(allOff);
  digitalWrite(relay_1, LOW);
  digitalWrite(Buzzer, LOW);
  ///////////////////////////////////////////////////////////////////
  if (EEPROM.read(5) == 0x00 || EEPROM.read(5) == 0xff) {
    EEPROM.write(5, 6);
  }
  loadTimerValueFromEEPROM();
  delay(100);
  loadRFIDCardsFromEEPROM();
  delay(100);
  lastStoredCardPosition = EEPROM.read(5);
  delay(100);
  if (!eepromIsAtInitial()) {
    loadMasterKey();
  }
  delay(2500);
}
//////////////////////////////////////////////////////////
void loop() {
  unsigned long oldTime = millis();
  unsigned long timeNow = millis();
  while (timeNow - oldTime <= BLINK_TIME) {
    // put your main code here, to run repeatedly:
    timeNow = millis();
    if (isSwitched) {
      switchColor(blue);
    } else {
      switchColor(allOff);
    }
    if (isMasterKey()) {
      // Start Programming Mode
      switchColor(blue);
      char keyStroke;
      do {
        keyStroke = customKeypad.getKey();
        if (keyStroke > 0) {
          buttonIsPressed(red);
          int key = int(keyStroke) - 48;
          switch (key) {
          case 1:
            // add new RFID card or cards!!
            addRFID_Cards();
            break;
          case 2:
            // set Password for users
            addAccessPassword();
            break;
          case 3:
            // remove all cards
            clearEEPROM();
            break;
          case 4:
            // change output relay time
            setOutputRelayEnergizeTime();
            break;
          case 5:
            changeMasterKey();
            break;
          }
        }
      } while (keyStroke != '#');
      switchColor(red);
    }
    if (rfidReader.PICC_IsNewCardPresent()) {
      if (rfidReader.PICC_ReadCardSerial()) {
        byte readCard[4];
        for (int i = 0; i < rfidReader.uid.size; i++) {
          readCard[i] = rfidReader.uid.uidByte[i];
        }
        rfidReader.PICC_HaltA();
        if (isMatched(readCard)) {
          energizeRelayOne(RELAY_1_TIME);
        } else {
          errorIndicator();
        }
      }
    }
  }
  isSwitched = !isSwitched;
}
