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
const char headerChar = '\n';
const char stopChar   = '\n';
const long maxCharLength = 200 + 2; // Actual max string length + start and stop characters
const int  freqOutputPin = 11; // OC2A output pin for ATmega328 boards
const long symbolCycles  = 1000000; // TODO: Number of base clock cycles per data symbol

// Main loop variables
String data_string;
bool data_is_valid;
int bitstream[maxCharLength];

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

// Simple data sending mechanism for testing
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

// Send a bitstream using FSK modulation scheme
void sendData(int * bitstream){
  int idx = 0;
  int bit = bitstream[idx];
  // While data is valid
  while(bit != 2){
    // Change OCR2A value to change output frequency
      // TODO

    // Calculate how long to wait to change OCR2A
      // TODO

    // Get next bit
    idx++;
    bit = bitstream[idx];
  }
}

// Insert the binary representation of character c into the bitstream at given pointer/index
void insertCharBits(char c, int * bitstream){
  for(uint8_t j = 0; j < 8; j++) {
    // Calculate index into bitstream
    long bitstream_idx = (7-j);
    // Store current bit
    bool cur_bit = (c & (1 << j));
    Serial.print((int) cur_bit);
    bitstream[bitstream_idx] = cur_bit;
  }
}

// Convert a given string to its binary format in an array of bools
void convertStringToBitStream(String _data_string, int * bitstream){
  Serial.println("Converting to bitstream");

  // Append header char (newline)
  insertCharBits(headerChar, bitstream);

  char cur_char;
  bool cur_char_bits[8];
  // Iterate through the characters in the string
  for(int i =1; i <= _data_string.length(); i++ ) {
    cur_char = _data_string[i];
    Serial.print("\nChar: ");
    Serial.println(cur_char);
    Serial.print("Binary: ");
    // Append current character's bits to bit representation
    insertCharBits(cur_char, &bitstream[i*8]);
  }

  // Append end of data char (newline)
  insertCharBits(stopChar, &bitstream[(_data_string.length() + 1) * 8]);
  bitstream[(_data_string.length() + 1) * 8 + 1] = 2; // End of valid data flag
}

void startDataTransmission(){
  // Setup timer and compare to trigger ISR at set symbol rate

  // Start timer
}

// Updating data ISR
// ISR(){
  // Check for valid data
  // Increment bitstream pointer and cast to int

  // Change OCR2A? value if valid

  // Stop timer for sending data if invalid

  // Reset something?
// }

void loop() {
  Serial.println("\nEnter data:");
  while (Serial.available() == 0) {} // Wait for data available
  data_string = Serial.readString(); // Read until timeout

  // Remove any \r \n whitespace at the end of the String
  data_string.trim();

  // Verify string isn't too long
  if (data_string.length() > maxCharLength){
    Serial.print("Error: Data length exceeds maximum character length of ");
    Serial.print(maxCharLength - 2);
    Serial.println(".");
  }
  // Else if valid data
  else {
    // Modulate and send the data string
    convertStringToBitStream(data_string, bitstream);
    sendDataSimple(data_string);
    // sendData(bitstream);
  }

  // Wait for serial things to finish
  delay(200);
}