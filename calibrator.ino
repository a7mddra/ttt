/*
 * Button-to-LED map calibrator.
 *
 * Upload this sketch, then press the physical buttons from left to right:
 *   1 2 3
 *   4 5 6
 *   7 8 9
 *
 * Each raw button slot lights one unique BLUE LED slot.
 *
 * IMPORTANT:
 * This file intentionally does NOT fix the swapped keypad columns.
 * It scans raw keypad columns as A3, A4, A5 so we can discover the real map.
 *
 * Send the observed output like:
 *   1:2, 2:5, 3:9, ...
 *
 * Meaning:
 *   physical button 1 lit physical LED 2
 *   physical button 2 lit physical LED 5
 *   physical button 3 lit physical LED 9
 */

#include <Arduino.h>

const byte ANODE_PINS[9] = {2, 3, 4, 5, 6, 7, 8, 9, 10};
const byte CATHODE_PINS[3] = {11, 12, 13};

const byte KEYPAD_ROW_PINS[3] = {A0, A1, A2};

// Raw physical column order. Do not swap these in the calibrator.
const byte KEYPAD_COL_PINS[3] = {A3, A4, A5};

struct LedRoute {
  byte cathodePin;
  byte anodePin;
};

// Blue-only routes from the previous raw LED sweep.
// If these are also wrong, that is fine: tell me what physical LED you see.
const LedRoute BLUE_LED[9] = {
  {13, 4},   // LED slot 1 blue
  {11, 4},   // LED slot 2 blue
  {12, 4},   // LED slot 3 blue
  {13, 7},   // LED slot 4 blue
  {11, 7},   // LED slot 5 blue
  {12, 7},   // LED slot 6 blue
  {13, 10},  // LED slot 7 blue
  {11, 10},  // LED slot 8 blue
  {12, 10},  // LED slot 9 blue
};

void allLedsOff() {
  for (byte i = 0; i < 9; i++) {
    digitalWrite(ANODE_PINS[i], LOW);
  }

  for (byte i = 0; i < 3; i++) {
    digitalWrite(CATHODE_PINS[i], LOW);
  }
}

void showBlueLed(byte slot) {
  allLedsOff();

  if (slot >= 9) {
    return;
  }

  digitalWrite(BLUE_LED[slot].anodePin, HIGH);
  digitalWrite(BLUE_LED[slot].cathodePin, HIGH);
}

int readRawKey() {
  for (byte r = 0; r < 3; r++) {
    for (byte x = 0; x < 3; x++) {
      pinMode(KEYPAD_ROW_PINS[x], INPUT);
    }

    pinMode(KEYPAD_ROW_PINS[r], OUTPUT);
    digitalWrite(KEYPAD_ROW_PINS[r], LOW);
    delayMicroseconds(100);

    for (byte c = 0; c < 3; c++) {
      if (digitalRead(KEYPAD_COL_PINS[c]) == LOW) {
        for (byte x = 0; x < 3; x++) {
          pinMode(KEYPAD_ROW_PINS[x], INPUT);
        }
        return r * 3 + c;
      }
    }
  }

  for (byte x = 0; x < 3; x++) {
    pinMode(KEYPAD_ROW_PINS[x], INPUT);
  }

  return -1;
}

int stableKey() {
  static int lastRaw = -1;
  static int stable = -1;
  static unsigned long changedAt = 0;

  int raw = readRawKey();
  unsigned long now = millis();

  if (raw != lastRaw) {
    lastRaw = raw;
    changedAt = now;
  }

  if (now - changedAt >= 35 && raw != stable) {
    stable = raw;
    return stable;
  }

  return -2;
}

void setup() {
  Serial.begin(9600);

  for (byte i = 0; i < 9; i++) {
    pinMode(ANODE_PINS[i], OUTPUT);
    digitalWrite(ANODE_PINS[i], LOW);
  }

  for (byte i = 0; i < 3; i++) {
    pinMode(CATHODE_PINS[i], OUTPUT);
    digitalWrite(CATHODE_PINS[i], LOW);
  }

  for (byte r = 0; r < 3; r++) {
    pinMode(KEYPAD_ROW_PINS[r], INPUT);
  }

  for (byte c = 0; c < 3; c++) {
    pinMode(KEYPAD_COL_PINS[c], INPUT_PULLUP);
  }

  delay(1000);

  Serial.println(F("\n=== BUTTON -> BLUE LED CALIBRATOR ==="));
  Serial.println(F("Press physical buttons 1..9 left-to-right, row-by-row."));
  Serial.println(F("Write what you SEE, like: 1:2, 2:5, 3:9"));
  Serial.println(F("Raw keypad columns are intentionally A3, A4, A5."));
  Serial.println(F("Only BLUE LEDs are used."));
}

void loop() {
  int key = stableKey();

  if (key == -2) {
    return;
  }

  if (key == -1) {
    allLedsOff();
    Serial.println(F("released"));
    return;
  }

  showBlueLed((byte)key);

  Serial.print(F("raw button slot "));
  Serial.print(key + 1);
  Serial.print(F(" -> blue LED slot "));
  Serial.println(key + 1);
}
