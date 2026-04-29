// Raw physical pins, no software swaps!
const byte rowPins[3] = {A0, A1, A2}; 
const byte colPins[3] = {A3, A4, A5}; 

String report[9];

void setup() {
  Serial.begin(9600);
  delay(2000); 
  
  // Set all columns to PULLUP
  for (int c = 0; c < 3; c++) {
    pinMode(colPins[c], INPUT_PULLUP);
  }

  Serial.println("\n=== KEYPAD RAW CALIBRATOR ===");
  Serial.println("Press the physical buttons one by one.");
  Serial.println("Start at Top-Left (1), go row by row, end at Bottom-Right (9).");
  Serial.println("-------------------------------------------------");

  for (int i = 1; i <= 9; i++) {
    Serial.print("Waiting for Button "); Serial.print(i); Serial.print(" ... ");
    
    int foundRow = -1;
    int foundCol = -1;
    
    // Blocking wait until a button is pressed
    while (foundRow == -1) {
      for (int r = 0; r < 3; r++) {
        // Float all rows
        for (int x = 0; x < 3; x++) pinMode(rowPins[x], INPUT);
        
        // Drive current row LOW
        pinMode(rowPins[r], OUTPUT);
        digitalWrite(rowPins[r], LOW);
        delayMicroseconds(100);
        
        // Check columns
        for (int c = 0; c < 3; c++) {
          if (digitalRead(colPins[c]) == LOW) {
            foundRow = r;
            foundCol = c;
            break;
          }
        }
        if (foundRow != -1) break;
      }
    }
    
    Serial.println("DETECTED!");
    
    // Format the pin names for the cheat sheet (A0-A2, A3-A5)
    String rName = "A" + String(foundRow);      
    String cName = "A" + String(foundCol + 3);  
    
    report[i - 1] = "Physical Button " + String(i) + " | Row Pin: " + rName + " | Col Pin: " + cName;
    
    // Wait for RELEASE so it doesn't double-trigger
    while(true) {
      bool released = true;
      if (digitalRead(colPins[foundCol]) == LOW) {
        released = false; // Still holding the button down
      }
      if (released) break;
      delay(10);
    }
    
    delay(200); // Extra debounce padding
  }
  
  // Dump the final cheat sheet
  Serial.println("\n==================================");
  Serial.println("    KEYPAD RAW CHEAT SHEET        ");
  Serial.println("==================================");
  for (int i = 0; i < 9; i++) {
    Serial.println(report[i]);
  }
  Serial.println("==================================");
  Serial.println("SYSTEM SHUTDOWN. SLEEP NOW.");
}

void loop() {
  // Done.
}
