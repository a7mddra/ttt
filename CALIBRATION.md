=== KEYPAD RAW CALIBRATION ===
=> here i pressed my key in-row left to right 1-9.
Physical Button 1 | Row Pin: A0 | Col Pin: A4
Physical Button 2 | Row Pin: A0 | Col Pin: A3
Physical Button 3 | Row Pin: A0 | Col Pin: A5
Physical Button 4 | Row Pin: A1 | Col Pin: A4
Physical Button 5 | Row Pin: A1 | Col Pin: A3
Physical Button 6 | Row Pin: A1 | Col Pin: A5
Physical Button 7 | Row Pin: A2 | Col Pin: A4
Physical Button 8 | Row Pin: A2 | Col Pin: A3
Physical Button 9 | Row Pin: A2 | Col Pin: A5

---

=== LEDS MATRIX RAW CALIBRATION ===
=> here i made the program to iterate through all 27 combinations and ask me each time "what is the color and position?".
01- BJT 11 | Anode 2 | Result: 8g
02- BJT 11 | Anode 3 | Result: 8r
03- BJT 11 | Anode 4 | Result: 2b
04- BJT 11 | Anode 5 | Result: 2g
05- BJT 11 | Anode 6 | Result: 2r
06- BJT 11 | Anode 7 | Result: 5b
07- BJT 11 | Anode 8 | Result: 5g
08- BJT 11 | Anode 9 | Result: 5r
09- BJT 11 | Anode 10 | Result: 8b
10- BJT 12 | Anode 2 | Result: 9g
11- BJT 12 | Anode 3 | Result: 9r
12- BJT 12 | Anode 4 | Result: 3b
13- BJT 12 | Anode 5 | Result: 3g
14- BJT 12 | Anode 6 | Result: 3r
15- BJT 12 | Anode 7 | Result: 6b
16- BJT 12 | Anode 8 | Result: 6g
17- BJT 12 | Anode 9 | Result: 6r
18- BJT 12 | Anode 10 | Result: 9b
19- BJT 13 | Anode 2 | Result: 7g
20- BJT 13 | Anode 3 | Result: 7r
21- BJT 13 | Anode 4 | Result: 1b
22- BJT 13 | Anode 5 | Result: 1g
23- BJT 13 | Anode 6 | Result: 1r
24- BJT 13 | Anode 7 | Result: 4b
25- BJT 13 | Anode 8 | Result: 4g
26- BJT 13 | Anode 9 | Result: 4r
27- BJT 13 | Anode 10 | Result: 7b

---

=== BUTTON TO BLUE LED CALIBRATION ===
=> pressed physical buttons left-to-right, row-by-row using calibrator.ino.
=> output means "physical button : physical blue LED seen".

1:1
2:3
3:2
4:4
5:6
6:5
7:7
8:9
9:8

Derived hardware interpretation:

brain board stays normal:

1 2 3
4 5 6
7 8 9

The button-to-blue test composes raw keypad input with physical blue LED output.
That means the observed swap belongs to the keypad/raw input side:

raw 1 -> logical 1
raw 2 -> logical 3
raw 3 -> logical 2
raw 4 -> logical 4
raw 5 -> logical 6
raw 6 -> logical 5
raw 7 -> logical 7
raw 8 -> logical 9
raw 9 -> logical 8

LED output is direct physical position:

logical 1 -> physical LED 1
logical 2 -> physical LED 2
logical 3 -> physical LED 3
logical 4 -> physical LED 4
logical 5 -> physical LED 5
logical 6 -> physical LED 6
logical 7 -> physical LED 7
logical 8 -> physical LED 8
logical 9 -> physical LED 9
