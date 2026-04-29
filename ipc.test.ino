#include <Arduino.h>

// --- Multiplexed Anodes ---
const byte RED_ROW[3] = {6, 9, 3};
const byte GREEN_ROW[3] = {5, 8, 2};
const byte BLUE_ROW[3] = {4, 7, 10};

// --- BJT Cathode Columns ---
const byte COL[3] = {11, 12, 13};

// Keypad Pins
const byte KP_ROW[3] = {A0, A1, A2};
const byte KP_COL[3] = {A3, A4, A5};

// --- GAME STATE ---
byte board[3][3] = {0};         // 0=Empty, 1=Blue, 2=Red
bool greenMask[3][3] = {false}; // True if part of a winning line
bool isBlueTurn = true;
bool gameOver = false;

// --- HARDWARE DRIVERS ---
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

// color: 1=Blue, 2=Red, 3=Green
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

// Scans the board dot-by-dot. Zero ghosting.
void renderBoard()
{
  for (byte r = 0; r < 3; r++)
  {
    for (byte c = 0; c < 3; c++)
    {
      if (greenMask[r][c])
      {
        lightOne(r, c, 3); // Winner is Green
        delayMicroseconds(1000);
      }
      else if (board[r][c] == 1)
      {
        lightOne(r, c, 1); // Player 1 is Blue
        delayMicroseconds(1000);
      }
      else if (board[r][c] == 2)
      {
        lightOne(r, c, 2); // Player 2 is Red
        delayMicroseconds(1000);
      }
    }
  }
  allOff(); // Blanking interval to prevent bleed
}

// --- LOGIC ---
void checkWin()
{
  byte lines[8][3][2] = {
      {{0, 0}, {0, 1}, {0, 2}}, {{1, 0}, {1, 1}, {1, 2}}, {{2, 0}, {2, 1}, {2, 2}}, // Rows
      {{0, 0}, {1, 0}, {2, 0}},
      {{0, 1}, {1, 1}, {2, 1}},
      {{0, 2}, {1, 2}, {2, 2}}, // Cols
      {{0, 0}, {1, 1}, {2, 2}},
      {{0, 2}, {1, 1}, {2, 0}} // Diagonals
  };

  for (int i = 0; i < 8; i++)
  {
    byte r1 = lines[i][0][0], c1 = lines[i][0][1];
    byte r2 = lines[i][1][0], c2 = lines[i][1][1];
    byte r3 = lines[i][2][0], c3 = lines[i][2][1];

    if (board[r1][c1] != 0 && board[r1][c1] == board[r2][c2] && board[r1][c1] == board[r3][c3])
    {
      greenMask[r1][c1] = true;
      greenMask[r2][c2] = true;
      greenMask[r3][c3] = true;
      gameOver = true;
    }
  }

  // Check for draw
  bool isFull = true;
  for (byte r = 0; r < 3; r++)
  {
    for (byte c = 0; c < 3; c++)
    {
      if (board[r][c] == 0)
        isFull = false;
    }
  }
  if (isFull && !gameOver)
    gameOver = true;
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
  delay(300); // Prevent accidental double-press
}

// --- KEYPAD INPUT ---
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

  if (raw != -1 && raw != lastRaw)
  {
    lastRaw = raw; // Lock until released

    if (gameOver)
    {
      resetGame();
      return;
    }

    byte physRow = raw / 3;
    byte rawCol = raw % 3;

    // Fix the swapped columns (A4 is Left, A3 is Middle, A5 is Right)
    byte physCol = 0;
    if (rawCol == 1)
      physCol = 0; // Left
    else if (rawCol == 0)
      physCol = 1; // Middle
    else if (rawCol == 2)
      physCol = 2; // Right

    if (board[physRow][physCol] == 0)
    {
      board[physRow][physCol] = isBlueTurn ? 1 : 2;
      isBlueTurn = !isBlueTurn;
      checkWin();
    }
  }

  if (raw == -1)
    lastRaw = -1; // Reset lock when released
}

// --- MAIN LOOP ---
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
  renderBoard();
}