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
const int  OCR2AHIGH  = 64; // 123 kHz
const int  OCR2ALOW   = 79; // 100 kHz
const int  OCR2AREST  = 60; //
const long maxCharLength = 60;
const int  freqOutputPin = 11; // OC2A output pin for ATmega328 boards
const long symbolCycles  = 1000000; // TODO: Number of base clock cycles per data symbol
const long numSyncTransmissions = 5;

// Main loop variables
String data_string;
bool data_is_valid;
uint8_t bitstream[maxCharLength * 8]; // Characters * bits per character
int string_len = 100;
uint8_t bitstream_pos = 0;
volatile long increm = 0;

bool sendingData = false;
volatile bool sendBit = false;

void setup()
{
    pinMode(freqOutputPin, OUTPUT);
    long baudrate = 115200;
    Serial.begin(baudrate);
    Serial.println("Transmitter Program Startup -- Baud Rate: \n");
    Serial.println(baudrate);
    Serial.println("Enter text to send:");

    bitstream[0] = OCR2AHIGH;
    bitstream[1] = OCR2ALOW;
    bitstream[2] = OCR2AHIGH;
    bitstream[3] = OCR2ALOW;
    bitstream[4] = OCR2AHIGH;
    bitstream[5] = OCR2ALOW;
    bitstream[6] = OCR2AHIGH;
    bitstream[7] = OCR2ALOW;
    bitstream[8] = OCR2AHIGH;


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
    // This value determines the output frequency - start at rest frequency
    OCR2A = OCR2AREST;
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
// TODO: Put this in an ISR that is trigger at the symbol rate
  // If hit end bit, disable Interrupts. To start data transmission, setup interrupt trigger
  // and start timer
  // make bitstream volatile variable
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

// Convert bit value to associated OCR2A value
int bitToOCR2A(bool bit){
  if(bit){
    return OCR2AHIGH;
  }else{
    return OCR2ALOW;
  }
}

// Insert the OCR2A value corresponding to the binary representation of character c into
// the bitstream at given pointer/index
void insertCharBits(char c, volatile uint8_t * bitstream){
  for(uint8_t j = 0; j < 8; j++) {
    // Calculate index into bitstream
    long bitstream_idx = (7-j);
    // Store current bit
    bool cur_bit = (c & (1 << j));
    Serial.print((int) cur_bit);
    bitstream[bitstream_idx] = bitToOCR2A(cur_bit);
  }
}

// Convert a given string to its binary format in an array of bools
void convertStringToBitStream(String _data_string, volatile uint8_t * bitstream){
  Serial.print("Converting \"");
  Serial.print(_data_string);
  Serial.println("\" to bitstream");
  Serial.print("Num Characters: ");
  Serial.print(_data_string.length());

  // Append header char (newline)
  // insertCharBits(headerChar, bitstream);

  char cur_char;
  bool cur_char_bits[8];
  // Iterate through the characters in the string
  for(int i =0; i < _data_string.length(); i++ ) {
    cur_char = _data_string[i];
    Serial.print("\nChar: ");
    Serial.println(cur_char);
    Serial.print("Binary (lsb to msb): ");
    // TODO: Convert to binary
    // Append current character's bits to bit representation
    insertCharBits(cur_char, &bitstream[i*8]);
  }
  Serial.println();
  // Append end of data char (newline)
  // insertCharBits(stopChar, &bitstream[(_data_string.length() + 1) * 8]);
  bitstream[(_data_string.length() + 1) * 8 + 1] = 2; // End of valid data flag
  string_len = (_data_string.length() + 1) * 8;
}

void startDataTransmission(){
  Serial.println("\nStarting data transmission");
  sendingData = true;
  bitstream_pos = 0;
  // Setup timer and compare to trigger ISR at set symbol rate

  TCNT1 = 0;
  // Set Timer 1 CTC operation mode with no prescaling.
  // Page 141 of datasheet
  // WGM2 bits 1 and 0 are in TCCR2A,
  // WGM2 bit 2 is in TCCR2B
  TCCR1A = bit (WGM12);   // WGM2:0 = 010 = CTC Mode

  // "Setting the PSRASY bit in GTCCR resets the prescaler.
  //  This allows the user to operate with a predictable prescaler." - datasheet
  GTCCR = bit (PSRASY);

  // Set Timer 1  No prescaling  (i.e. prescale division = 1)
  // Page 143 of datasheet
  // CS1(2:0) = 001: Use CPU clock with no prescaling
  // CS1 bits 2:0 are all in TCCR1B
  // TCCR1A = bit (CS12) | bit (CS10); // CS1(2:0) = 101 = 1024 prescaler
  TCCR1B = bit (CS10);              // CS1(2:0) = 001 = No   prescaler

  // Make sure Compare-match register A interrupt for timer 1 is enabled
  TIMSK1 = bit (OCIE1A);
;
  // This value determines the output frequency - start at 8 MHz?
  OCR1A = OCR2AREST;

  // Start Timer?
}

// Updating data ISR
ISR(TIMER1_COMPA_vect){
  sendBit = true;
}

void loop() {
  if(sendingData){
    if(sendBit){
      if (bitstream_pos >= string_len-8) {
        bitstream_pos = 0;
        // Disable interrupt if done transmitting
        TIMSK1 = 0;
        OCR2A = OCR2AREST;
        sendingData = false;
      } else {
        OCR2A = bitstream[bitstream_pos];
        sendBit = false;
        // Serial.print("bitstream pos: ");
        // Serial.println(bitstream_pos);
        // Serial.print("string_len: ");
        // Serial.println(string_len);
        bitstream_pos++;
      }
    }
  }
  // Else if not sending data
  else {
    // Serial.println("Not sending data.");
    // delay(500);
    // startDataTransmission();

    Serial.println("\nEnter data:");
    // Wait for user to enter text in serial port
    while (Serial.available() == 0) {}

    data_string = Serial.readString(); // Read user input
    // Remove any \r \n whitespace at the end of the String
    data_string.trim();

    // Verify string isn't too long
    if (data_string.length() > maxCharLength){
      Serial.print("Error: Data length exceeds maximum character length of ");
      Serial.print(maxCharLength);
      Serial.println(". Try again.");
    }
    // Else if valid data, modulate and send the data string
    else {
      // Convert given string to bitstream representation
      convertStringToBitStream(data_string, bitstream);
      // Display datastream
      // for(int i=0; i<string_len; i++){
      //   // If beginning of new byte, print on newline
      //   if(i != 0 && i % 8 == 0){
      //     Serial.println();
      //   }
      //   Serial.print((bool) bitstream[i]);

      // }
      // Start timer to output symbols at given rate
      startDataTransmission();
    }
  }

}