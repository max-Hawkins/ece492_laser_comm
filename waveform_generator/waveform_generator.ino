#define out_pin 3

void setup() {
  pinMode(out_pin,128);
  // Frequency of ~3.921kHz - Prescaler value of 8
  // To alter this, change the last 3 bits
  TCCR2B = TCCR2B & B11111000 | B00000001;
}

void loop() {
  analogWrite(out_pin,128);
}
