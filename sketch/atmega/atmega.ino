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

// --- Multiplexed Anodes ---
const byte RED_ROW[3] = {6, 9, 3};
const byte GREEN_ROW[3] = {5, 8, 2};
const byte BLUE_ROW[3] = {4, 7, 10};

// --- BJT Cathode Columns ---
const byte COL[3] = {11, 12, 13};

const byte KP_ROW[3] = {A0, A1, A2};
const byte KP_COL[3] = {A3, A4, A5};

byte board[3][3] = {0};
bool greenMask[3][3] = {false};
bool isBlueTurn = true;
bool gameOver = false;

// 0 = none
// 1 = row 0
// 2 = row 1
// 3 = row 2
// 4 = col 0
// 5 = col 1
// 6 = col 2
// 7 = diag 0,0 -> 1,1 -> 2,2
// 8 = diag 0,2 -> 1,1 -> 2,0
// 9 = draw
byte winType = 0;

void allOff()
{
  for (byte i = 0; i < 3; i++)
  {
    digitalWrite(COL[i], LOW);
    digitalWrite(RED_ROW[i], LOW);
    digitalWrite(GREEN_ROW[i], LOW);
    digitalWrite(BLUE_ROW[i], LOW);
  }
}

void lightOne(byte r, byte c, byte color)
{
  allOff();

  if (color == 1)
    digitalWrite(BLUE_ROW[r], HIGH);
  if (color == 2)
    digitalWrite(RED_ROW[r], HIGH);
  if (color == 3)
    digitalWrite(GREEN_ROW[r], HIGH);

  digitalWrite(COL[c], HIGH);
}

void renderBoard()
{
  for (byte r = 0; r < 3; r++)
  {
    for (byte c = 0; c < 3; c++)
    {
      if (greenMask[r][c])
      {
        lightOne(r, c, 3);
        delayMicroseconds(1000);
      }
      else if (board[r][c] == 1)
      {
        lightOne(r, c, 1);
        delayMicroseconds(1000);
      }
      else if (board[r][c] == 2)
      {
        lightOne(r, c, 2);
        delayMicroseconds(1000);
      }
    }
  }
  allOff();
}

// ======================================================
//  GAME-OVER GREEN RENDERER
// ======================================================

void greenRow0()
{
  allOff();

  digitalWrite(GREEN_ROW[0], HIGH);

  digitalWrite(COL[0], HIGH);
  digitalWrite(COL[1], HIGH);
  digitalWrite(COL[2], HIGH);
}

void greenRow1()
{
  allOff();

  digitalWrite(GREEN_ROW[1], HIGH);

  digitalWrite(COL[0], HIGH);
  digitalWrite(COL[1], HIGH);
  digitalWrite(COL[2], HIGH);
}

void greenRow2()
{
  allOff();

  digitalWrite(GREEN_ROW[2], HIGH);

  digitalWrite(COL[0], HIGH);
  digitalWrite(COL[1], HIGH);
  digitalWrite(COL[2], HIGH);
}

void greenCol0()
{
  allOff();

  digitalWrite(GREEN_ROW[0], HIGH);
  digitalWrite(GREEN_ROW[1], HIGH);
  digitalWrite(GREEN_ROW[2], HIGH);

  digitalWrite(COL[0], HIGH);
}

void greenCol1()
{
  allOff();

  digitalWrite(GREEN_ROW[0], HIGH);
  digitalWrite(GREEN_ROW[1], HIGH);
  digitalWrite(GREEN_ROW[2], HIGH);

  digitalWrite(COL[1], HIGH);
}

void greenCol2()
{
  allOff();

  digitalWrite(GREEN_ROW[0], HIGH);
  digitalWrite(GREEN_ROW[1], HIGH);
  digitalWrite(GREEN_ROW[2], HIGH);

  digitalWrite(COL[2], HIGH);
}

void greenOne00()
{
  allOff();
  digitalWrite(GREEN_ROW[0], HIGH);
  digitalWrite(COL[0], HIGH);
}

void greenOne11()
{
  allOff();
  digitalWrite(GREEN_ROW[1], HIGH);
  digitalWrite(COL[1], HIGH);
}

void greenOne22()
{
  allOff();
  digitalWrite(GREEN_ROW[2], HIGH);
  digitalWrite(COL[2], HIGH);
}

void greenOne02()
{
  allOff();
  digitalWrite(GREEN_ROW[0], HIGH);
  digitalWrite(COL[2], HIGH);
}

void greenOne20()
{
  allOff();
  digitalWrite(GREEN_ROW[2], HIGH);
  digitalWrite(COL[0], HIGH);
}

void renderDiag1ForeverStep()
{
  static byte step = 0;

  if (step == 0)
    greenOne00();
  if (step == 1)
    greenOne11();
  if (step == 2)
    greenOne22();

  delayMicroseconds(1500);

  step++;
  if (step >= 3)
    step = 0;
}

void renderDiag2ForeverStep()
{
  static byte step = 0;

  if (step == 0)
    greenOne02();
  if (step == 1)
    greenOne11();
  if (step == 2)
    greenOne20();

  delayMicroseconds(1500);

  step++;
  if (step >= 3)
    step = 0;
}

void renderGameOver()
{
  if (winType == 1)
  {
    greenRow0();
    return;
  }

  if (winType == 2)
  {
    greenRow1();
    return;
  }

  if (winType == 3)
  {
    greenRow2();
    return;
  }

  if (winType == 4)
  {
    greenCol0();
    return;
  }

  if (winType == 5)
  {
    greenCol1();
    return;
  }

  if (winType == 6)
  {
    greenCol2();
    return;
  }

  if (winType == 7)
  {
    renderDiag1ForeverStep();
    return;
  }

  if (winType == 8)
  {
    renderDiag2ForeverStep();
    return;
  }

  if (winType == 9)
  {
    renderBoard();
    return;
  }

  renderBoard();
}

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
        v = 1;
      }
      else if (board[r][c] == 1)
      {
        v = 2;
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
    board[r][c] = 2;
  }
}

// ======================================================
//  WIN CHECK
// ======================================================

void checkWin()
{
  // Row 0
  if (board[0][0] != 0 && board[0][0] == board[0][1] && board[0][0] == board[0][2])
  {
    greenMask[0][0] = true;
    greenMask[0][1] = true;
    greenMask[0][2] = true;
    winType = 1;
    gameOver = true;
    return;
  }

  // Row 1
  if (board[1][0] != 0 && board[1][0] == board[1][1] && board[1][0] == board[1][2])
  {
    greenMask[1][0] = true;
    greenMask[1][1] = true;
    greenMask[1][2] = true;
    winType = 2;
    gameOver = true;
    return;
  }

  // Row 2
  if (board[2][0] != 0 && board[2][0] == board[2][1] && board[2][0] == board[2][2])
  {
    greenMask[2][0] = true;
    greenMask[2][1] = true;
    greenMask[2][2] = true;
    winType = 3;
    gameOver = true;
    return;
  }

  // Col 0
  if (board[0][0] != 0 && board[0][0] == board[1][0] && board[0][0] == board[2][0])
  {
    greenMask[0][0] = true;
    greenMask[1][0] = true;
    greenMask[2][0] = true;
    winType = 4;
    gameOver = true;
    return;
  }

  // Col 1
  if (board[0][1] != 0 && board[0][1] == board[1][1] && board[0][1] == board[2][1])
  {
    greenMask[0][1] = true;
    greenMask[1][1] = true;
    greenMask[2][1] = true;
    winType = 5;
    gameOver = true;
    return;
  }

  // Col 2
  if (board[0][2] != 0 && board[0][2] == board[1][2] && board[0][2] == board[2][2])
  {
    greenMask[0][2] = true;
    greenMask[1][2] = true;
    greenMask[2][2] = true;
    winType = 6;
    gameOver = true;
    return;
  }

  // Diag 1
  if (board[0][0] != 0 && board[0][0] == board[1][1] && board[0][0] == board[2][2])
  {
    greenMask[0][0] = true;
    greenMask[1][1] = true;
    greenMask[2][2] = true;
    winType = 7;
    gameOver = true;
    return;
  }

  // Diag 2
  if (board[0][2] != 0 && board[0][2] == board[1][1] && board[0][2] == board[2][0])
  {
    greenMask[0][2] = true;
    greenMask[1][1] = true;
    greenMask[2][0] = true;
    winType = 8;
    gameOver = true;
    return;
  }

  bool isFull = true;

  for (byte r = 0; r < 3; r++)
  {
    for (byte c = 0; c < 3; c++)
    {
      if (board[r][c] == 0)
        isFull = false;
    }
  }

  if (isFull)
  {
    winType = 9;
    gameOver = true;
  }
}

void resetGame()
{
  for (byte r = 0; r < 3; r++)
  {
    for (byte c = 0; c < 3; c++)
    {
      board[r][c] = 0;
      greenMask[r][c] = false;
    }
  }

  isBlueTurn = true;
  gameOver = false;
  winType = 0;

  delay(300);
}

int readRawKey()
{
  for (byte r = 0; r < 3; r++)
  {
    for (byte x = 0; x < 3; x++)
      pinMode(KP_ROW[x], INPUT);
    pinMode(KP_ROW[r], OUTPUT);
    digitalWrite(KP_ROW[r], LOW);
    delayMicroseconds(100);

    for (byte c = 0; c < 3; c++)
    {
      if (digitalRead(KP_COL[c]) == LOW)
      {
        for (byte x = 0; x < 3; x++)
          pinMode(KP_ROW[x], INPUT);
        return r * 3 + c;
      }
    }
  }

  for (byte x = 0; x < 3; x++)
    pinMode(KP_ROW[x], INPUT);
  return -1;
}

void processInput()
{
  static int lastRaw = -1;
  int raw = readRawKey();

  if (gameOver)
  {
    if (raw != -1 && raw != lastRaw)
    {
      lastRaw = raw;
      resetGame();
    }

    if (raw == -1)
      lastRaw = -1;

    return;
  }

  if (raw != -1 && raw != lastRaw)
  {
    lastRaw = raw;

    byte physRow = raw / 3;
    byte rawCol = raw % 3;

    byte physCol = 0;
    if (rawCol == 1)
      physCol = 0;
    else if (rawCol == 0)
      physCol = 1;
    else if (rawCol == 2)
      physCol = 2;

    if (board[physRow][physCol] == 0)
    {
      // Blue user move
      board[physRow][physCol] = 1;
      checkWin();

      if (gameOver)
        return;

      // Red AI move
      playRedFromLut();
      checkWin();

      isBlueTurn = true;
    }
  }

  if (raw == -1)
    lastRaw = -1;
}

void setup()
{
  for (byte i = 0; i < 3; i++)
  {
    pinMode(RED_ROW[i], OUTPUT);
    pinMode(GREEN_ROW[i], OUTPUT);
    pinMode(BLUE_ROW[i], OUTPUT);
    pinMode(COL[i], OUTPUT);

    pinMode(KP_ROW[i], INPUT);
    pinMode(KP_COL[i], INPUT_PULLUP);
  }

  allOff();
}

void loop()
{
  processInput();

  if (gameOver)
  {
    renderGameOver();
    return;
  }

  renderBoard();
}
