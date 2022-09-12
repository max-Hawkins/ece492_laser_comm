// Transmitter/Waveform Generation

// Uses Timer 2 to generate a signal for a particular frequency on pin 11

// If you change the prescale value, it affects CS22, CS21, and CS20
// For a given prescale value, the eight-bit number that you
// load into OCR2A determines the frequency.
// With no prescaling, an OCR2B value of 3 causes the output pin to
// toggle the value every four CPU clock cycles. That is, the
// period is equal to eight slock cycles.
// With F_CPU = 16 MHz, the result is 2 MHz.
// Author: Max Hawkins
// Date: Fall 2022
//----------------------------------------------------------------------------

// Constants
const long maxCharLength = 200 + 2; // Actual max string length + start and stop characters
const int freqOutputPin = 11; // OC2A output pin for ATmega328 boards

// Main loop variables
String data_string;
bool data_is_valid;
bool bitstream[maxCharLength];

void setup()
{
    pinMode(freqOutputPin, OUTPUT);
    Serial.begin(115200);
    Serial.println("Transmitter Program Startup\n");

    // Set Timer 2 CTC mode with no prescaling.  OC2A toggles on compare match
    // WGM22:0 = 010: CTC Mode, toggle OC
    // WGM2 bits 1 and 0 are in TCCR2A,
    // WGM2 bit 2 is in TCCR2B
    // COM2A0 sets OC2A (arduino pin 11 on Uno or Duemilanove) to toggle on compare match
    TCCR2A = ((1 << WGM21) | (1 << COM2A0));

    // Set Timer 2  No prescaling  (i.e. prescale division = 1)
    // CS22:0 = 001: Use CPU clock with no prescaling
    // CS2 bits 2:0 are all in TCCR2B
    TCCR2B = bit (CS20);

    // Make sure Compare-match register A interrupt for timer 2 is disabled
    TIMSK2 = 0;
    // This value determines the output frequency - start at 8 MHz?
    OCR2A = 1;
}

void sendDataSimple(String _data_string){
  _data_string.trim(); // Remove any \r \n whitespace at the end of the String
  if (_data_string == "0") {
    OCR2A = 4;
    Serial.println("OCR2A = 4; Data should be 0.");
  } else {
    OCR2A = 1;
    Serial.println("OCR2A = 1; Data should be 1.");
  }
}

bool convertStringToBitStream(String _data_string, bool * bitstream){
  Serial.println("Converting to bitstream");
  // Remove any \r \n whitespace at the end of the String
  _data_string.trim();

  int data_len = _data_string.length();
  // Verify string isn't too long
  if (data_len + 2 > maxCharLength){
    Serial.print("Error: Data length exceeds maximum character length of ");
    Serial.print(maxCharLength - 2);
    Serial.println(".");

    return false; // Return error status
  }

  char cur_char;
  bool cur_char_bits[8];
  // Iterate through the characters in the string
  for(int i =0; i < _data_string.length(); i++ ) {
    cur_char = _data_string[i];
    Serial.print("Char: ");
    Serial.println(cur_char);

    // Convert to bit representation

    // Add current char bits to output bitstream
  }
  return true;
}

void loop() {
  Serial.println("\nEnter data:");
  while (Serial.available() == 0) {} // Wait for data available
  data_string = Serial.readString(); // Read until timeout
  data_is_valid = convertStringToBitStream(data_string, bitstream); // Modulate and send the data string
  if (data_is_valid) {
    sendDataSimple(data_string);
  }
  delay(200);
}