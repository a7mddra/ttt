const int anodes[9] = {2, 3, 4, 5, 6, 7, 8, 9, 10};
const int cathodes[3] = {11, 12, 13};

String report[27]; // Array to hold your cheat sheet
int step = 0;

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 9; i++) pinMode(anodes[i], OUTPUT);
  for (int i = 0; i < 3; i++) {
    pinMode(cathodes[i], OUTPUT);
    digitalWrite(cathodes[i], LOW);
  }
  
  delay(2000);
  Serial.println("--- RAW DATA CALIBRATOR ---");
  Serial.println("Type POS(1-9) + COLOR(r/g/b). Example: '1r' or '9b'");
  Serial.println("Type '0' if the LED is completely off.");
  Serial.println("-----------------------------------");

  for (int row = 0; row < 3; row++) {
    digitalWrite(cathodes[row], HIGH); 
    
    for (int col = 0; col < 9; col++) {
      digitalWrite(anodes[col], HIGH); 
      
      Serial.print("BJT "); Serial.print(cathodes[row]);
      Serial.print(" + Anode "); Serial.print(anodes[col]);
      Serial.print(" ---> ? ");
      
      // Wait for you to type the answer
      while (Serial.available() == 0) { delay(10); } 
      String ans = Serial.readStringUntil('\n');
      ans.trim(); 
      Serial.println(ans); // Echo what you typed
      
      // Save it directly to the cheat sheet
      report[step] = "BJT " + String(cathodes[row]) + " | Anode " + String(anodes[col]) + " | Result: " + ans;
      step++;
      
      digitalWrite(anodes[col], LOW); 
    }
    digitalWrite(cathodes[row], LOW); 
  }
  
  // Dump the final cheat sheet
  Serial.println("\n==================================");
  Serial.println("   YOUR RAW CHEAT SHEET REPORT    ");
  Serial.println("==================================");
  for (int i = 0; i < 27; i++) {
    Serial.println(report[i]);
  }
  Serial.println("==================================");
  Serial.println("Copy this, paste it to me, and GO TO SLEEP!");
}

void loop() {
  // Done.
}