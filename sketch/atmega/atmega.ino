#include <Arduino.h>
#include <avr/pgmspace.h>
#include "lut.h"

/*
                              ┌─────────────┐
       (RESET) 1k to 5V  1  ->│ •           │<- 28  Keypad
    (RX to Nano) Serial  2  ->│             │<- 27  Keypad
    (TX to Nano) Serial  3  ->│             │<- 26  Keypad
             (Anode) D2  4  ->│             │<- 25  Keypad
             (Anode) D3  5  ->│             │<- 24  Keypad
             (Anode) D4  6  ->│             │<- 23  Keypad
                 5V VCC  7  ->│             │<- 22  GND
                    GND  8  ->│ ATmega 328p │<- 21  AREF
          16MHz Crystal  9  ->│             │<- 20  5V AVCC
          16MHz Crystal 10  ->│             │<- 19  D13 (BJT Cathode Row)
             (Anode) D5 11  ->│             │<- 18  D12 (BJT Cathode Row)
             (Anode) D6 12  ->│             │<- 17  D11 (BJT Cathode Row)
             (Anode) D7 13  ->│             │<- 16  D10 (Anode)
             (Anode) D8 14  ->│             │<- 15  D9  (Anode)
                              └─────────────┘
*/


// ==========================================
// 1. HARDWARE MAPPING
// ==========================================
const byte all_anodes[9] = { 2, 3, 4, 5, 6, 7, 8, 9, 10 };

const byte anode_map[3][3] = {
  { 7, 10, 9 },
  { 4, 3, 2 },
  { 5, 8, 6 }
};

const byte cathode_map[3] = { 12, 13, 11 };

const byte KR0 = A0;
const byte KR1 = A3;
const byte KR2 = A2;
const byte KC0 = A5;
const byte KC1 = A1;
const byte KC2 = A4;

// ==========================================
// 2. STATE & BUFFERS
// ==========================================
byte matrix[3][3] = { 0 };
byte board[3][3] = { 0 };
bool greenMask[3][3] = { false }; // Used to track winning line locations now, not actual green color

const byte MODE_AI = 0;
const byte MODE_PVP = 1;
byte gameMode = MODE_AI;

byte currentPlayer = 1;
bool gameOver = false;
int lastRawKey = -1;

byte moveCount = 0;
byte firstUserMove = 255;
byte firstAiMove = 255;

// ==========================================
// 3. LOW LEVEL LED ENGINE (Software GPU)
// ==========================================
void allOff() {
  for (byte i = 0; i < 3; i++) digitalWrite(cathode_map[i], LOW);
  for (byte i = 0; i < 9; i++) {
    pinMode(all_anodes[i], OUTPUT);
    digitalWrite(all_anodes[i], LOW);
  }
}

void renderFrames(int frames) {
  for (int f = 0; f < frames; f++) {
    for (byte r = 0; r < 3; r++) {
      for (byte c = 0; c < 3; c++) {
        byte color = matrix[r][c];
        if (color > 0 && color <= 3) {
          byte anodePin = anode_map[r][color - 1];
          byte cathodePin = cathode_map[c];

          digitalWrite(cathodePin, HIGH);
          digitalWrite(anodePin, HIGH);

          delayMicroseconds(500);

          digitalWrite(anodePin, LOW);
          digitalWrite(cathodePin, LOW);
        }
      }
    }
  }
}

// Banned Green: Just copy the board directly to the visual matrix
void updateDisplayBuffer() {
  for (byte r = 0; r < 3; r++) {
    for (byte c = 0; c < 3; c++) {
      matrix[r][c] = board[r][c]; 
    }
  }
}

// ==========================================
// 4. KEYPAD ENGINE
// ==========================================
void keypadIdle() {
  pinMode(KR0, INPUT);
  pinMode(KR1, INPUT);
  pinMode(KR2, INPUT);
  pinMode(KC0, INPUT_PULLUP);
  pinMode(KC1, INPUT_PULLUP);
  pinMode(KC2, INPUT_PULLUP);
}

int readRawKey() {
  allOff();
  keypadIdle();

  pinMode(KR0, OUTPUT);
  digitalWrite(KR0, LOW);
  delayMicroseconds(80);
  if (digitalRead(KC0) == LOW) { keypadIdle(); return 0; }
  if (digitalRead(KC1) == LOW) { keypadIdle(); return 1; }
  if (digitalRead(KC2) == LOW) { keypadIdle(); return 2; }
  pinMode(KR0, INPUT);

  pinMode(KR1, OUTPUT);
  digitalWrite(KR1, LOW);
  delayMicroseconds(80);
  if (digitalRead(KC0) == LOW) { keypadIdle(); return 3; }
  if (digitalRead(KC1) == LOW) { keypadIdle(); return 4; }
  if (digitalRead(KC2) == LOW) { keypadIdle(); return 5; }
  pinMode(KR1, INPUT);

  pinMode(KR2, OUTPUT);
  digitalWrite(KR2, LOW);
  delayMicroseconds(80);
  if (digitalRead(KC0) == LOW) { keypadIdle(); return 6; }
  if (digitalRead(KC1) == LOW) { keypadIdle(); return 7; }
  if (digitalRead(KC2) == LOW) { keypadIdle(); return 8; }
  pinMode(KR2, INPUT);

  keypadIdle();
  return -1;
}

// Quickly checks if ANY button is pressed for the hard reboot
bool checkAnyButtonFast() {
  pinMode(KR0, OUTPUT); digitalWrite(KR0, LOW);
  pinMode(KR1, OUTPUT); digitalWrite(KR1, LOW);
  pinMode(KR2, OUTPUT); digitalWrite(KR2, LOW);
  delayMicroseconds(50);
  
  bool pressed = (digitalRead(KC0) == LOW || digitalRead(KC1) == LOW || digitalRead(KC2) == LOW);
  
  pinMode(KR0, INPUT); pinMode(KR1, INPUT); pinMode(KR2, INPUT);
  return pressed;
}

// ==========================================
// 5. GAME LOGIC & WIN SCREENS
// ==========================================
void resetGame() {
  for (byte r = 0; r < 3; r++) {
    for (byte c = 0; c < 3; c++) {
      board[r][c] = 0;
      greenMask[r][c] = false;
    }
  }

  gameMode = MODE_AI;
  currentPlayer = 1;
  gameOver = false;

  moveCount = 0;
  firstUserMove = 255;
  firstAiMove = 255;

  updateDisplayBuffer();
}

void checkWin() {
  bool won = false;
  // Rows
  for (byte r = 0; r < 3; r++) {
    if (board[r][0] && board[r][0] == board[r][1] && board[r][0] == board[r][2]) {
      greenMask[r][0] = greenMask[r][1] = greenMask[r][2] = true;
      won = true;
    }
  }
  // Cols
  for (byte c = 0; c < 3; c++) {
    if (board[0][c] && board[0][c] == board[1][c] && board[0][c] == board[2][c]) {
      greenMask[0][c] = greenMask[1][c] = greenMask[2][c] = true;
      won = true;
    }
  }
  // Diagonals
  if (board[0][0] && board[0][0] == board[1][1] && board[0][0] == board[2][2]) {
    greenMask[0][0] = greenMask[1][1] = greenMask[2][2] = true;
    won = true;
  }
  if (board[0][2] && board[0][2] == board[1][1] && board[0][2] == board[2][0]) {
    greenMask[0][2] = greenMask[1][1] = greenMask[2][0] = true;
    won = true;
  }

  if (won) {
    gameOver = true;
    return;
  }

  // Check Draw
  bool isFull = true;
  for (byte r = 0; r < 3; r++) {
    for (byte c = 0; c < 3; c++) {
      if (board[r][c] == 0) isFull = false;
    }
  }
  if (isFull) gameOver = true;
}

// THE DEAD BRAIN THREAD: Blinks the winning line natively
void winScreenRenderer_Thread() {
  renderFrames(100); 
  
  bool blinkState = true;
  int blinkCounter = 0;

  while (true) {
    // 1. Setup the blink animation
    for (byte r = 0; r < 3; r++) {
      for (byte c = 0; c < 3; c++) {
        byte val = board[r][c];

        if (greenMask[r][c]) {
          matrix[r][c] = blinkState ? val : 0; // Blink the winner
        } else {
          matrix[r][c] = val; // Keep history solid
        }
      }
    }

    // 2. Draw it
    renderFrames(10); 
    
    // 3. Timing for the blink
    blinkCounter++;
    if (blinkCounter > 15) { 
      blinkState = !blinkState;
      blinkCounter = 0;
    }

    // 4. Hard Reboot on press
    if (checkAnyButtonFast()) {
       asm volatile ("  jmp 0");
    }
  }
}

// ==========================================
// 6. LUT AI ENGINE
// ==========================================
uint16_t encodeBoardForLut() {
  uint16_t key = 0;
  uint16_t pow3 = 1;

  for (byte r = 0; r < 3; r++) {
    for (byte c = 0; c < 3; c++) {
      byte v = 0;
      // LUT expects: 0=empty, 1=Red AI, 2=Blue User
      if (board[r][c] == 2) v = 1; // AI is Red
      if (board[r][c] == 1) v = 2; // User is Blue
      
      key += (uint16_t)v * pow3;
      pow3 *= 3;
    }
  }
  return key;
}

void playAIMove() {
  uint16_t key = encodeBoardForLut();
  
  // Strictly read from Flash memory
  uint8_t packed = pgm_read_byte_near(&BEST_MOVE[key]);
  
  if (packed == 0xFF) return; // Failsafe
  
  byte move = packed >> 4; // Extract primary move from high nibble
  
  if (move <= 8) {
    byte r = move / 3;
    byte c = move % 3;
    
    board[r][c] = 2; // Red AI places piece
    firstAiMove = move; // Store in case user triggers PvP override
  }
}

// ==========================================
// 7. MAIN LOOP & EVENT HANDLER
// ==========================================
void processInput() {
  int raw = readRawKey();

  if (raw != -1 && raw != lastRawKey) {

    if (gameOver) {
      resetGame(); // Failsafe
    } else {
      byte r = raw / 3;
      byte c = raw % 3;

      // Double Tap Override
      if (gameMode == MODE_AI && moveCount == 2 && raw == firstUserMove) {
        gameMode = MODE_PVP;
        board[firstAiMove / 3][firstAiMove % 3] = 0;
        moveCount = 1;
        currentPlayer = 2;
        updateDisplayBuffer();
        lastRawKey = raw;
        return;
      }

      if (board[r][c] == 0) {
        board[r][c] = currentPlayer;
        moveCount++;

        if (moveCount == 1) firstUserMove = raw;

        checkWin();
        if (gameOver) {
          updateDisplayBuffer();
          winScreenRenderer_Thread(); // CPU TRAPPED HERE
        }

        if (!gameOver) {
          if (gameMode == MODE_AI) {
            updateDisplayBuffer();
            renderFrames(10);
            
            playAIMove();
            moveCount++;
            
            checkWin();
            if (gameOver) {
              updateDisplayBuffer();
              winScreenRenderer_Thread(); // CPU TRAPPED HERE
            }
          } else {
            currentPlayer = (currentPlayer == 1) ? 2 : 1;
          }
        }
      }
    }

    updateDisplayBuffer();
  }

  if (raw == -1) {
    lastRawKey = -1;
  } else {
    lastRawKey = raw;
  }
}

void setup() {
  Serial.begin(9600); // Safe to leave running!
  for (byte i = 0; i < 3; i++) pinMode(cathode_map[i], OUTPUT);
  keypadIdle();
  resetGame();
}

void loop() {
  renderFrames(5);
  processInput();
}