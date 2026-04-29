/*
 * Personalized unbeatable tic-tac-toe for ATmega328P / Arduino AVR.
 *
 * CRITICAL CALIBRATION NOTE:
 * The keypad and LED matrix columns are not wired in logical 1-2-3 order.
 * Do not "clean up" these pin tables. They are hardcoded from CALIBRATION.md.
 *
 * CLI prototype convention is preserved:
 *   r = AI / red LED
 *   b = user / blue LED
 *   g = winning line / green LED
 */

#include <Arduino.h>

const char EMPTY = '0';
const char AI = 'r';
const char USER = 'b';

const byte CELL_COUNT = 9;
const byte LINE_COUNT = 8;

const byte WIN_LINES[LINE_COUNT][3] = {
  {0, 1, 2},
  {3, 4, 5},
  {6, 7, 8},
  {0, 3, 6},
  {1, 4, 7},
  {2, 5, 8},
  {0, 4, 8},
  {2, 4, 6},
};

const byte MOVE_ORDER[CELL_COUNT] = {4, 0, 2, 6, 8, 1, 3, 5, 7};

// Raw keypad rows are normal top-to-bottom.
const byte KEYPAD_ROW_PINS[3] = {A0, A1, A2};

// CRITICAL: physical left-to-right columns from CALIBRATION.md are A4, A3, A5.
const byte KEYPAD_COL_PINS[3] = {A4, A3, A5};

const byte ANODE_PINS[9] = {2, 3, 4, 5, 6, 7, 8, 9, 10};
const byte CATHODE_PINS[3] = {11, 12, 13};

const byte COLOR_RED = 0;
const byte COLOR_GREEN = 1;
const byte COLOR_BLUE = 2;
const byte COLOR_OFF = 255;

struct LedRoute {
  byte cathodePin;
  byte anodePin;
};

// [cell 1..9 as index 0..8][red, green, blue].
// Every entry is copied from CALIBRATION.md's raw "BJT + Anode => Result" table.
const LedRoute LED_ROUTE[CELL_COUNT][3] = {
  {{13, 6}, {13, 5}, {13, 4}},    // 1r, 1g, 1b
  {{11, 6}, {11, 5}, {11, 4}},    // 2r, 2g, 2b
  {{12, 6}, {12, 5}, {12, 4}},    // 3r, 3g, 3b
  {{13, 9}, {13, 8}, {13, 7}},    // 4r, 4g, 4b
  {{11, 9}, {11, 8}, {11, 7}},    // 5r, 5g, 5b
  {{12, 9}, {12, 8}, {12, 7}},    // 6r, 6g, 6b
  {{13, 3}, {13, 2}, {13, 10}},   // 7r, 7g, 7b
  {{11, 3}, {11, 2}, {11, 10}},   // 8r, 8g, 8b
  {{12, 3}, {12, 2}, {12, 10}},   // 9r, 9g, 9b
};

struct GameResult {
  char winner;
  bool full;
  int8_t line[3];
};

struct TreeScore {
  int8_t guarantee;       // AI win = +1, draw = 0, AI loss = -1.
  uint8_t terminalDepth;  // Smaller for wins, larger for losses.
  uint32_t wins;
  uint32_t draws;
  uint32_t losses;
};

struct MoveScore {
  int8_t move;
  TreeScore tree;
  int8_t aiThreats;
  int8_t aiForks;
  int8_t userThreats;
  int8_t userForks;
  uint8_t position;
  uint16_t salt;
};

const byte MODE_PLAYING = 0;
const byte MODE_WIN = 1;
const byte MODE_DRAW = 2;

char board[CELL_COUNT];
char visibleBoard[CELL_COUNT];
byte gameMode = MODE_PLAYING;
char gameWinner = EMPTY;
int8_t activeLine[3] = {-1, -1, -1};
uint16_t searchRefreshCounter = 0;

char serialBuffer[16];
byte serialLength = 0;

void clearMatrixPins() {
  for (byte i = 0; i < 9; i++) {
    digitalWrite(ANODE_PINS[i], LOW);
  }

  for (byte i = 0; i < 3; i++) {
    digitalWrite(CATHODE_PINS[i], LOW);
  }
}

bool onActiveLine(byte pos) {
  return activeLine[0] == pos || activeLine[1] == pos || activeLine[2] == pos;
}

byte colorForSymbol(char symbol) {
  if (symbol == AI) {
    return COLOR_RED;
  }

  if (symbol == USER) {
    return COLOR_BLUE;
  }

  if (symbol == 'g') {
    return COLOR_GREEN;
  }

  return COLOR_OFF;
}

char displaySymbolForCell(byte pos) {
  if (gameMode == MODE_WIN && onActiveLine(pos)) {
    return 'g';
  }

  if (gameMode == MODE_DRAW) {
    bool blinkOn = ((millis() / 300UL) % 2UL) == 0;
    return blinkOn ? 'g' : EMPTY;
  }

  return visibleBoard[pos];
}

void refreshDisplay() {
  static byte cathodeIndex = 0;
  static unsigned long lastStepMicros = 0;

  unsigned long now = micros();
  if (now - lastStepMicros < 1200UL) {
    return;
  }
  lastStepMicros = now;

  clearMatrixPins();

  byte activeCathode = CATHODE_PINS[cathodeIndex];
  bool hasAnyLed = false;

  for (byte cell = 0; cell < CELL_COUNT; cell++) {
    byte color = colorForSymbol(displaySymbolForCell(cell));
    if (color == COLOR_OFF) {
      continue;
    }

    LedRoute route = LED_ROUTE[cell][color];
    if (route.cathodePin == activeCathode) {
      digitalWrite(route.anodePin, HIGH);
      hasAnyLed = true;
    }
  }

  if (hasAnyLed) {
    digitalWrite(activeCathode, HIGH);
  }

  cathodeIndex++;
  if (cathodeIndex >= 3) {
    cathodeIndex = 0;
  }
}

void serviceDuringSearch() {
  searchRefreshCounter++;
  if (searchRefreshCounter >= 64) {
    searchRefreshCounter = 0;
    refreshDisplay();
  }
}

void syncVisibleBoard() {
  for (byte i = 0; i < CELL_COUNT; i++) {
    visibleBoard[i] = board[i];
  }
}

void printBoard() {
  for (byte row = 0; row < 3; row++) {
    for (byte col = 0; col < 3; col++) {
      byte pos = row * 3 + col;
      char out = visibleBoard[pos];
      if (gameMode == MODE_WIN && onActiveLine(pos)) {
        out = 'g';
      }

      if (col > 0) {
        Serial.print(' ');
      }
      Serial.print(out);
    }
    Serial.println();
  }
}

void resetGame() {
  for (byte i = 0; i < CELL_COUNT; i++) {
    board[i] = EMPTY;
    visibleBoard[i] = EMPTY;
  }

  gameMode = MODE_PLAYING;
  gameWinner = EMPTY;
  activeLine[0] = -1;
  activeLine[1] = -1;
  activeLine[2] = -1;

  Serial.println(F("\n=== NEW GAME ==="));
  Serial.println(F("AI=r, user=b, win line=g"));
  Serial.println(F("Use keypad or Serial commands b1..b9. Type reset to clear."));
  printBoard();
}

GameResult inspectBoard() {
  GameResult result;
  result.winner = EMPTY;
  result.full = true;
  result.line[0] = -1;
  result.line[1] = -1;
  result.line[2] = -1;

  for (byte i = 0; i < LINE_COUNT; i++) {
    byte a = WIN_LINES[i][0];
    byte b = WIN_LINES[i][1];
    byte c = WIN_LINES[i][2];
    char mark = board[a];

    if (mark != EMPTY && mark == board[b] && mark == board[c]) {
      result.winner = mark;
      result.line[0] = a;
      result.line[1] = b;
      result.line[2] = c;
      return result;
    }
  }

  for (byte i = 0; i < CELL_COUNT; i++) {
    if (board[i] == EMPTY) {
      result.full = false;
      break;
    }
  }

  return result;
}

int8_t lineThreats(char who) {
  int8_t count = 0;

  for (byte i = 0; i < LINE_COUNT; i++) {
    int8_t mine = 0;
    int8_t blank = 0;

    for (byte j = 0; j < 3; j++) {
      char cell = board[WIN_LINES[i][j]];
      if (cell == who) {
        mine++;
      } else if (cell == EMPTY) {
        blank++;
      }
    }

    if (mine == 2 && blank == 1) {
      count++;
    }
  }

  return count;
}

int8_t forks(char who) {
  int8_t count = 0;

  for (byte pos = 0; pos < CELL_COUNT; pos++) {
    if (board[pos] != EMPTY) {
      continue;
    }

    board[pos] = who;
    bool immediateWin = inspectBoard().winner == who;
    int8_t threats = lineThreats(who);
    board[pos] = EMPTY;

    if (!immediateWin && threats >= 2) {
      count++;
    }
  }

  return count;
}

uint8_t positionValue(byte pos) {
  if (pos == 4) {
    return 4;
  }

  if (pos == 0 || pos == 2 || pos == 6 || pos == 8) {
    return 3;
  }

  return 2;
}

TreeScore terminalScore(int8_t guarantee, uint8_t depth, uint32_t wins, uint32_t draws, uint32_t losses) {
  TreeScore score;
  score.guarantee = guarantee;
  score.terminalDepth = depth;
  score.wins = wins;
  score.draws = draws;
  score.losses = losses;
  return score;
}

bool betterTreeForTurn(const TreeScore& child, const TreeScore& best, char turn) {
  if (child.guarantee != best.guarantee) {
    return turn == AI ? child.guarantee > best.guarantee : child.guarantee < best.guarantee;
  }

  if (turn == AI) {
    if (child.guarantee == 1) {
      return child.terminalDepth < best.terminalDepth;
    }

    if (child.guarantee == -1) {
      return child.terminalDepth > best.terminalDepth;
    }

    return child.wins > best.wins ||
           (child.wins == best.wins && child.losses < best.losses);
  }

  if (child.guarantee == -1) {
    return child.terminalDepth < best.terminalDepth;
  }

  if (child.guarantee == 1) {
    return child.terminalDepth > best.terminalDepth;
  }

  return child.losses > best.losses ||
         (child.losses == best.losses && child.wins < best.wins);
}

TreeScore solve(char turn, uint8_t depth) {
  serviceDuringSearch();

  GameResult state = inspectBoard();
  if (state.winner == AI) {
    return terminalScore(1, depth, 1, 0, 0);
  }

  if (state.winner == USER) {
    return terminalScore(-1, depth, 0, 0, 1);
  }

  if (state.full) {
    return terminalScore(0, depth, 0, 1, 0);
  }

  TreeScore best;
  bool hasBest = false;
  uint32_t wins = 0;
  uint32_t draws = 0;
  uint32_t losses = 0;

  for (byte i = 0; i < CELL_COUNT; i++) {
    byte move = MOVE_ORDER[i];
    if (board[move] != EMPTY) {
      continue;
    }

    board[move] = turn;
    TreeScore child = solve(turn == AI ? USER : AI, depth + 1);
    board[move] = EMPTY;

    wins += child.wins;
    draws += child.draws;
    losses += child.losses;

    if (!hasBest || betterTreeForTurn(child, best, turn)) {
      best = child;
      hasBest = true;
    }
  }

  best.wins = wins;
  best.draws = draws;
  best.losses = losses;
  return best;
}

bool betterAiMove(const MoveScore& a, const MoveScore& b) {
  if (b.move == -1) {
    return true;
  }

  if (a.tree.guarantee != b.tree.guarantee) {
    return a.tree.guarantee > b.tree.guarantee;
  }

  if (a.tree.guarantee == 1 && a.tree.terminalDepth != b.tree.terminalDepth) {
    return a.tree.terminalDepth < b.tree.terminalDepth;
  }

  if (a.tree.guarantee == -1 && a.tree.terminalDepth != b.tree.terminalDepth) {
    return a.tree.terminalDepth > b.tree.terminalDepth;
  }

  if (a.tree.wins != b.tree.wins) {
    return a.tree.wins > b.tree.wins;
  }

  if (a.aiForks != b.aiForks) {
    return a.aiForks > b.aiForks;
  }

  if (a.aiThreats != b.aiThreats) {
    return a.aiThreats > b.aiThreats;
  }

  if (a.userThreats != b.userThreats) {
    return a.userThreats < b.userThreats;
  }

  if (a.userForks != b.userForks) {
    return a.userForks < b.userForks;
  }

  if (a.tree.losses != b.tree.losses) {
    return a.tree.losses < b.tree.losses;
  }

  if (a.tree.draws != b.tree.draws) {
    return a.tree.draws < b.tree.draws;
  }

  if (a.position != b.position) {
    return a.position > b.position;
  }

  return a.salt > b.salt;
}

int8_t openingReplyIfKnown() {
  byte occupied = 0;
  for (byte i = 0; i < CELL_COUNT; i++) {
    if (board[i] != EMPTY) {
      occupied++;
    }
  }

  if (occupied != 1) {
    return -1;
  }

  // Embedded shortcut from note.md: known-safe first reply, then full personality search.
  if (board[4] == EMPTY) {
    return 4;
  }

  const byte corners[4] = {0, 2, 6, 8};
  byte start = random(0, 4);
  for (byte i = 0; i < 4; i++) {
    byte pos = corners[(start + i) % 4];
    if (board[pos] == EMPTY) {
      return pos;
    }
  }

  return -1;
}

MoveScore chooseAiMove() {
  MoveScore best;
  best.move = -1;

  int8_t opening = openingReplyIfKnown();
  if (opening != -1) {
    best.move = opening;
    best.tree = terminalScore(0, 1, 0, 0, 0);
    best.aiThreats = 0;
    best.aiForks = 0;
    best.userThreats = 0;
    best.userForks = 0;
    best.position = positionValue(opening);
    best.salt = 0;
    return best;
  }

  searchRefreshCounter = 0;

  for (byte i = 0; i < CELL_COUNT; i++) {
    byte move = MOVE_ORDER[i];
    if (board[move] != EMPTY) {
      continue;
    }

    board[move] = AI;
    TreeScore tree = solve(USER, 1);

    MoveScore candidate;
    candidate.move = move;
    candidate.tree = tree;
    candidate.aiThreats = lineThreats(AI);
    candidate.aiForks = forks(AI);
    candidate.userThreats = lineThreats(USER);
    candidate.userForks = forks(USER);
    candidate.position = positionValue(move);
    candidate.salt = (uint16_t)random(0, 65535);

    board[move] = EMPTY;

    if (betterAiMove(candidate, best)) {
      best = candidate;
    }
  }

  return best;
}

void copyResultLine(const GameResult& result) {
  activeLine[0] = result.line[0];
  activeLine[1] = result.line[1];
  activeLine[2] = result.line[2];
}

bool finishIfGameEnded() {
  GameResult result = inspectBoard();

  if (result.winner != EMPTY) {
    syncVisibleBoard();
    gameMode = MODE_WIN;
    gameWinner = result.winner;
    copyResultLine(result);

    Serial.print(result.winner);
    Serial.println(F(" wins"));
    printBoard();
    return true;
  }

  if (result.full) {
    syncVisibleBoard();
    gameMode = MODE_DRAW;
    gameWinner = EMPTY;

    Serial.println(F("draw"));
    printBoard();
    return true;
  }

  return false;
}

void printMoveStats(const MoveScore& score) {
  Serial.print('r');
  Serial.print(score.move + 1);
  Serial.print(F(" | guarantee="));

  if (score.tree.guarantee == 1) {
    Serial.print(F("win"));
  } else if (score.tree.guarantee == 0) {
    Serial.print(F("draw"));
  } else {
    Serial.print(F("loss"));
  }

  Serial.print(F(" | leaves w/d/l="));
  Serial.print(score.tree.wins);
  Serial.print('/');
  Serial.print(score.tree.draws);
  Serial.print('/');
  Serial.print(score.tree.losses);
  Serial.print(F(" | threats="));
  Serial.print(score.aiThreats);
  Serial.print(F(" | forks="));
  Serial.println(score.aiForks);
}

void processUserMove(byte pos) {
  if (gameMode != MODE_PLAYING) {
    Serial.println(F("game over, reset first"));
    return;
  }

  if (pos >= CELL_COUNT) {
    return;
  }

  if (board[pos] != EMPTY) {
    Serial.print(F("cell "));
    Serial.print(pos + 1);
    Serial.println(F(" is busy"));
    return;
  }

  board[pos] = USER;
  syncVisibleBoard();

  Serial.print('b');
  Serial.println(pos + 1);
  printBoard();

  if (finishIfGameEnded()) {
    return;
  }

  Serial.println(F("AI thinking..."));
  MoveScore ai = chooseAiMove();
  if (ai.move == -1) {
    gameMode = MODE_DRAW;
    Serial.println(F("draw"));
    return;
  }

  board[ai.move] = AI;
  syncVisibleBoard();
  printMoveStats(ai);

  if (!finishIfGameEnded()) {
    printBoard();
  }
}

int8_t readRawKey() {
  for (byte r = 0; r < 3; r++) {
    for (byte x = 0; x < 3; x++) {
      pinMode(KEYPAD_ROW_PINS[x], INPUT);
    }

    pinMode(KEYPAD_ROW_PINS[r], OUTPUT);
    digitalWrite(KEYPAD_ROW_PINS[r], LOW);
    delayMicroseconds(80);

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

int8_t scanKeypad() {
  const unsigned long debounceMs = 35;
  static int8_t lastRaw = -1;
  static int8_t stable = -1;
  static unsigned long changedAt = 0;

  int8_t raw = readRawKey();
  unsigned long now = millis();

  if (raw != lastRaw) {
    lastRaw = raw;
    changedAt = now;
  }

  if (now - changedAt >= debounceMs && raw != stable) {
    stable = raw;
    if (stable != -1) {
      return stable;
    }
  }

  return -1;
}

bool commandEquals(const char* command, const char* word) {
  while (*command && *word) {
    char a = *command;
    if (a >= 'A' && a <= 'Z') {
      a = a - 'A' + 'a';
    }
    if (a != *word) {
      return false;
    }
    command++;
    word++;
  }

  return *command == '\0' && *word == '\0';
}

void processSerialCommand(const char* command) {
  if (command[0] == '\0') {
    return;
  }

  if (commandEquals(command, "reset") || commandEquals(command, "new")) {
    resetGame();
    return;
  }

  if (commandEquals(command, "board")) {
    printBoard();
    return;
  }

  if (command[0] == USER && command[1] >= '1' && command[1] <= '9' && command[2] == '\0') {
    processUserMove(command[1] - '1');
    return;
  }

  Serial.println(F("bad command, use b1..b9, board, or reset"));
}

void handleSerial() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      serialBuffer[serialLength] = '\0';
      processSerialCommand(serialBuffer);
      serialLength = 0;
      continue;
    }

    if (serialLength < (byte)(sizeof(serialBuffer) - 1)) {
      serialBuffer[serialLength] = c;
      serialLength++;
    }
  }
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

  randomSeed(micros());
  delay(1000);

  Serial.println(F("\n=== PERSONALIZED TTT ATMEGA ==="));
  Serial.println(F("Calibration is hardcoded from CALIBRATION.md."));
  Serial.println(F("Keypad physical columns: A4, A3, A5."));
  Serial.println(F("LED routes are raw BJT/anode pairs, not logical rows."));

  resetGame();
}

void loop() {
  refreshDisplay();
  handleSerial();

  int8_t key = scanKeypad();
  if (key != -1) {
    processUserMove((byte)key);
  }
}
