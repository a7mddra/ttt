// Define the pins connected to your ATmega328P (Pins 23-28 on the DIP chip)
const byte rowPins[3] = {A0, A3, A2}; 
const byte colPins[3] = {A5, A1, A4}; 

void setup() {
  Serial.begin(9600);
  delay(1000); 

  Serial.println("\n=== KEYPAD MATRIX SCANNER ===");
  Serial.println("Press any button to see its Row and Col...");
  Serial.println("-------------------------------------------");

  // Set all columns to INPUT_PULLUP (they will read HIGH by default)
  for (int c = 0; c < 3; c++) {
    pinMode(colPins[c], INPUT_PULLUP);
  }

  // Set all rows to INPUT (high impedance) so they don't interfere yet
  for (int r = 0; r < 3; r++) {
    pinMode(rowPins[r], INPUT);
  }
}

void loop() {
  // Scan row by row
  for (int r = 0; r < 3; r++) {
    
    // Set the current row as OUTPUT and pull it LOW
    pinMode(rowPins[r], OUTPUT);
    digitalWrite(rowPins[r], LOW);

    // Check all columns to see if any are pulled LOW by a button press
    for (int c = 0; c < 3; c++) {
      if (digitalRead(colPins[c]) == LOW) {
        
        Serial.print("Button Pressed -> Row: "); 
        Serial.print(r); 
        Serial.print(" | Col: "); 
        Serial.println(c);
        
        // Simple delay to debounce the button and prevent serial spam
        delay(250); 
      }
    }

    // Set the row back to INPUT (high impedance) before moving to the next row
    pinMode(rowPins[r], INPUT);
  }
}