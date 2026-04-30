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

// ---------- LED PINS ----------

// Physical rows, color anodes
const byte R0 = 6;
const byte R1 = 9;
const byte R2 = 3;

const byte G0 = 5;
const byte G1 = 8;
const byte G2 = 2;

const byte BL0 = 4;
const byte BL1 = 7;
const byte BL2 = 10;

// Cathode columns through BJTs
const byte C0 = 11;
const byte C1 = 12;
const byte C2 = 13;

// Keypad
const byte KR0 = A0;
const byte KR1 = A1;
const byte KR2 = A2;

const byte KC0 = A3;
const byte KC1 = A4;
const byte KC2 = A5;

// ---------- GAME STATE ----------

byte board[3][3] = {0};          // 0 empty, 1 blue, 2 red
bool greenMask[3][3] = {false};  // winning LEDs
bool isBlueTurn = true;
bool gameOver = false;

// ======================================================
//  LUT: 0=empty, 1=red AI, 2=blue user
//  Board: 0=empty, 1=blue user, 2=red AI
// ======================================================

uint16_t encodeBoardForLut()
{
  uint16_t key = 0;
  uint16_t pow3 = 1;

  for (byte r = 0; r < 3; r++)
  {
    for (byte c = 0; c < 3; c++)
    {
      byte v = 0;

      if (board[r][c] == 2)
      {
        v = 1; // red AI in LUT
      }
      else if (board[r][c] == 1)
      {
        v = 2; // blue user in LUT
      }

      key += (uint16_t)v * pow3;
      pow3 *= 3;
    }
  }

  return key;
}

byte firstEmptyMove()
{
  for (byte pos = 0; pos < 9; pos++)
  {
    byte r = pos / 3;
    byte c = pos % 3;

    if (board[r][c] == 0)
    {
      return pos;
    }
  }

  return 255;
}

byte aiMoveFromLut()
{
  uint16_t key = encodeBoardForLut();
  uint8_t packed = pgm_read_byte(&BEST_MOVE[key]);

  if (packed == 0xFF)
  {
    return 255;
  }

  byte move = packed >> 4;

  if (move > 8)
  {
    return 255;
  }

  byte r = move / 3;
  byte c = move % 3;

  if (board[r][c] != 0)
  {
    return 255;
  }

  return move;
}

void playRedFromLut()
{
  byte move = aiMoveFromLut();

  if (move == 255)
  {
    move = firstEmptyMove();
  }

  if (move == 255)
  {
    return;
  }

  byte r = move / 3;
  byte c = move % 3;

  if (board[r][c] == 0)
  {
    board[r][c] = 2; // red AI
  }
}

// ---------- LOW LEVEL LED CONTROL ----------

void anodesHiZ()
{
  // First write LOW, then input.
  // This avoids enabling internal pullups accidentally.
  digitalWrite(R0, LOW); pinMode(R0, INPUT);
  digitalWrite(R1, LOW); pinMode(R1, INPUT);
  digitalWrite(R2, LOW); pinMode(R2, INPUT);

  digitalWrite(G0, LOW); pinMode(G0, INPUT);
  digitalWrite(G1, LOW); pinMode(G1, INPUT);
  digitalWrite(G2, LOW); pinMode(G2, INPUT);

digitalWrite(BL0, LOW); pinMode(BL0, INPUT);
digitalWrite(BL1, LOW); pinMode(BL1, INPUT);
digitalWrite(BL2, LOW); pinMode(BL2, INPUT);
}

void cathodesOff()
{
  digitalWrite(C0, LOW);
  digitalWrite(C1, LOW);
  digitalWrite(C2, LOW);
}

void allOff()
{
  cathodesOff();
  anodesHiZ();
}

// color: 1 blue, 2 red, 3 green
void oneLedHard(byte r, byte c, byte color)
{
  allOff();

  // Small blanking time: lets BJT/storage/capacitance settle.
  delayMicroseconds(30);

  byte anodePin = 255;
  byte cathodePin = 255;

  // ---------- HARD CODED LED MAP ----------

  if (r == 0 && c == 0)
  {
    if (color == 1) anodePin = BL0;
    if (color == 2) anodePin = R0;
    if (color == 3) anodePin = G0;
    cathodePin = C0;
  }
  else if (r == 0 && c == 1)
  {
    if (color == 1) anodePin = BL0;
    if (color == 2) anodePin = R0;
    if (color == 3) anodePin = G0;
    cathodePin = C1;
  }
  else if (r == 0 && c == 2)
  {
    if (color == 1) anodePin = BL0;
    if (color == 2) anodePin = R0;
    if (color == 3) anodePin = G0;
    cathodePin = C2;
  }

  else if (r == 1 && c == 0)
  {
    if (color == 1) anodePin = BL1;
    if (color == 2) anodePin = R1;
    if (color == 3) anodePin = G1;
    cathodePin = C0;
  }
  else if (r == 1 && c == 1)
  {
    if (color == 1) anodePin = BL1;
    if (color == 2) anodePin = R1;
    if (color == 3) anodePin = G1;
    cathodePin = C1;
  }
  else if (r == 1 && c == 2)
  {
    if (color == 1) anodePin = BL1;
    if (color == 2) anodePin = R1;
    if (color == 3) anodePin = G1;
    cathodePin = C2;
  }

  else if (r == 2 && c == 0)
  {
    if (color == 1) anodePin = BL2;
    if (color == 2) anodePin = R2;
    if (color == 3) anodePin = G2;
    cathodePin = C0;
  }
  else if (r == 2 && c == 1)
  {
    if (color == 1) anodePin = BL2;
    if (color == 2) anodePin = R2;
    if (color == 3) anodePin = G2;
    cathodePin = C1;
  }
  else if (r == 2 && c == 2)
  {
    if (color == 1) anodePin = BL2;
    if (color == 2) anodePin = R2;
    if (color == 3) anodePin = G2;
    cathodePin = C2;
  }

  if (anodePin == 255 || cathodePin == 255)
  {
    allOff();
    return;
  }

  // Turn anode on first, then sink cathode.
  pinMode(anodePin, OUTPUT);
  digitalWrite(anodePin, HIGH);

  delayMicroseconds(5);

  digitalWrite(cathodePin, HIGH);

  delayMicroseconds(300);

  allOff();
}

// ---------- DISPLAY ----------

void renderBoard()
{
  // Hardcoded-ish safe display scan.
  // Empty LEDs are skipped.

  if (greenMask[0][0]) oneLedHard(0, 0, 3);
  else if (board[0][0] == 1) oneLedHard(0, 0, 1);
  else if (board[0][0] == 2) oneLedHard(0, 0, 2);

  if (greenMask[0][1]) oneLedHard(0, 1, 3);
  else if (board[0][1] == 1) oneLedHard(0, 1, 1);
  else if (board[0][1] == 2) oneLedHard(0, 1, 2);

  if (greenMask[0][2]) oneLedHard(0, 2, 3);
  else if (board[0][2] == 1) oneLedHard(0, 2, 1);
  else if (board[0][2] == 2) oneLedHard(0, 2, 2);


  if (greenMask[1][0]) oneLedHard(1, 0, 3);
  else if (board[1][0] == 1) oneLedHard(1, 0, 1);
  else if (board[1][0] == 2) oneLedHard(1, 0, 2);

  if (greenMask[1][1]) oneLedHard(1, 1, 3);
  else if (board[1][1] == 1) oneLedHard(1, 1, 1);
  else if (board[1][1] == 2) oneLedHard(1, 1, 2);

  if (greenMask[1][2]) oneLedHard(1, 2, 3);
  else if (board[1][2] == 1) oneLedHard(1, 2, 1);
  else if (board[1][2] == 2) oneLedHard(1, 2, 2);


  if (greenMask[2][0]) oneLedHard(2, 0, 3);
  else if (board[2][0] == 1) oneLedHard(2, 0, 1);
  else if (board[2][0] == 2) oneLedHard(2, 0, 2);

  if (greenMask[2][1]) oneLedHard(2, 1, 3);
  else if (board[2][1] == 1) oneLedHard(2, 1, 1);
  else if (board[2][1] == 2) oneLedHard(2, 1, 2);

  if (greenMask[2][2]) oneLedHard(2, 2, 3);
  else if (board[2][2] == 1) oneLedHard(2, 2, 1);
  else if (board[2][2] == 2) oneLedHard(2, 2, 2);

  allOff();
}

// ---------- GAME LOGIC ----------

void checkWin()
{
  // clear old green mask first
  for (byte r = 0; r < 3; r++)
  {
    for (byte c = 0; c < 3; c++)
    {
      greenMask[r][c] = false;
    }
  }

  gameOver = false;

  // rows
  if (board[0][0] && board[0][0] == board[0][1] && board[0][0] == board[0][2])
  {
    greenMask[0][0] = greenMask[0][1] = greenMask[0][2] = true;
    gameOver = true;
  }

  if (board[1][0] && board[1][0] == board[1][1] && board[1][0] == board[1][2])
  {
    greenMask[1][0] = greenMask[1][1] = greenMask[1][2] = true;
    gameOver = true;
  }

  if (board[2][0] && board[2][0] == board[2][1] && board[2][0] == board[2][2])
  {
    greenMask[2][0] = greenMask[2][1] = greenMask[2][2] = true;
    gameOver = true;
  }

  // columns
  if (board[0][0] && board[0][0] == board[1][0] && board[0][0] == board[2][0])
  {
    greenMask[0][0] = greenMask[1][0] = greenMask[2][0] = true;
    gameOver = true;
  }

  if (board[0][1] && board[0][1] == board[1][1] && board[0][1] == board[2][1])
  {
    greenMask[0][1] = greenMask[1][1] = greenMask[2][1] = true;
    gameOver = true;
  }

  if (board[0][2] && board[0][2] == board[1][2] && board[0][2] == board[2][2])
  {
    greenMask[0][2] = greenMask[1][2] = greenMask[2][2] = true;
    gameOver = true;
  }

  // diagonals
  if (board[0][0] && board[0][0] == board[1][1] && board[0][0] == board[2][2])
  {
    greenMask[0][0] = greenMask[1][1] = greenMask[2][2] = true;
    gameOver = true;
  }

  if (board[0][2] && board[0][2] == board[1][1] && board[0][2] == board[2][0])
  {
    greenMask[0][2] = greenMask[1][1] = greenMask[2][0] = true;
    gameOver = true;
  }

  // draw
  bool full = true;

  for (byte r = 0; r < 3; r++)
  {
    for (byte c = 0; c < 3; c++)
    {
      if (board[r][c] == 0) full = false;
    }
  }

  if (full && !gameOver)
  {
    gameOver = true;
  }
}

void resetGame()
{
  allOff();

  board[0][0] = 0; board[0][1] = 0; board[0][2] = 0;
  board[1][0] = 0; board[1][1] = 0; board[1][2] = 0;
  board[2][0] = 0; board[2][1] = 0; board[2][2] = 0;

  greenMask[0][0] = false; greenMask[0][1] = false; greenMask[0][2] = false;
  greenMask[1][0] = false; greenMask[1][1] = false; greenMask[1][2] = false;
  greenMask[2][0] = false; greenMask[2][1] = false; greenMask[2][2] = false;

  isBlueTurn = true; // harmless now, but keep it
  gameOver = false;
}

// ---------- KEYPAD ----------

void keypadIdle()
{
  pinMode(KR0, INPUT);
  pinMode(KR1, INPUT);
  pinMode(KR2, INPUT);

  pinMode(KC0, INPUT_PULLUP);
  pinMode(KC1, INPUT_PULLUP);
  pinMode(KC2, INPUT_PULLUP);
}

int readRawKey()
{
  // Important: shut LED matrix completely off before touching keypad.
  allOff();

  keypadIdle();

  // row 0
  pinMode(KR0, OUTPUT);
  digitalWrite(KR0, LOW);
  delayMicroseconds(80);

  if (digitalRead(KC0) == LOW) { keypadIdle(); return 0; }
  if (digitalRead(KC1) == LOW) { keypadIdle(); return 1; }
  if (digitalRead(KC2) == LOW) { keypadIdle(); return 2; }

  pinMode(KR0, INPUT);

  // row 1
  pinMode(KR1, OUTPUT);
  digitalWrite(KR1, LOW);
  delayMicroseconds(80);

  if (digitalRead(KC0) == LOW) { keypadIdle(); return 3; }
  if (digitalRead(KC1) == LOW) { keypadIdle(); return 4; }
  if (digitalRead(KC2) == LOW) { keypadIdle(); return 5; }

  pinMode(KR1, INPUT);

  // row 2
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

void processInput()
{
  static int lastRaw = -1;

  int raw = readRawKey();

  if (raw != -1 && raw != lastRaw)
  {
    lastRaw = raw;

    if (gameOver)
    {
      resetGame();
      return;
    }

    byte physRow = raw / 3;
    byte rawCol = raw % 3;

    // Your swapped column fix:
    // A4 = left, A3 = middle, A5 = right
    byte physCol;

    if (rawCol == 1)
      physCol = 0;
    else if (rawCol == 0)
      physCol = 1;
    else
      physCol = 2;

    // User can only play blue.
    if (board[physRow][physCol] == 0)
    {
      board[physRow][physCol] = 1; // blue user
      checkWin();

      if (!gameOver)
      {
        // Let the display breathe for a tiny moment before AI moves.
        // This is optional, but feels better on real LEDs.
        renderBoard();
        renderBoard();

        playRedFromLut(); // red AI
        checkWin();
      }
    }
  }

  if (raw == -1)
  {
    lastRaw = -1;
  }
}

// ---------- SETUP / LOOP ----------

void setup()
{
  // cathodes
  pinMode(C0, OUTPUT);
  pinMode(C1, OUTPUT);
  pinMode(C2, OUTPUT);

  cathodesOff();
  anodesHiZ();

  keypadIdle();
}

void loop()
{
  processInput();

  // Draw several refresh passes per input read.
  // This keeps brightness stable and reduces keypad/display fighting.
  renderBoard();
  renderBoard();
  renderBoard();
}
