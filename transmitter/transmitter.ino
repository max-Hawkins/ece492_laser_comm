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
const long numSyncTransmissions = 5;

// Main loop variables
bool synchronized;
String data_string;
bool data_is_valid;
volatile int bitstream[maxCharLength];
volatile int string_len = 100;
volatile uint8_t bitstream_pos = 0;
volatile long increm = 0;

volatile bool sendingData = false;
volatile bool sendBit = false;

void setup()
{
    pinMode(freqOutputPin, OUTPUT);
    Serial.begin(115200);
    Serial.println("Transmitter Program Startup\n");

    synchronized = false;

    bitstream[0] = 50;
    bitstream[1] = 60;
    bitstream[2] = 50;
    bitstream[3] = 60;
    bitstream[4] = 50;
    bitstream[5] = 60;
    bitstream[6] = 50;
    bitstream[7] = 60;




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
    OCR2A = 2;
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

// Insert the binary representation of character c into the bitstream at given pointer/index
void insertCharBits(char c, volatile int * bitstream){
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
void convertStringToBitStream(String _data_string, volatile int * bitstream){
  Serial.println("Converting to bitstream");

  // Append header char (newline)
  insertCharBits(headerChar, bitstream);

  char cur_char;
  bool cur_char_bits[8];
  // Iterate through the characters in the string
  for(int i =0; i < _data_string.length(); i++ ) {
    cur_char = _data_string[i];
    Serial.print("\nChar: ");
    Serial.println(cur_char);
    Serial.print("Binary: ");
    delay(200);
    // Append current character's bits to bit representation
    insertCharBits(cur_char, &bitstream[i*8]);
  }
  Serial.println();
  // Append end of data char (newline)
  insertCharBits(stopChar, &bitstream[(_data_string.length() + 1) * 8]);
  bitstream[(_data_string.length() + 1) * 8 + 1] = 2; // End of valid data flag
  string_len = (_data_string.length() + 1) * 8;
}

void startDataTransmission(){
  Serial.println("Starting data transmission");
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
  OCR1A = 12;

  // Start Timer?
}

// Updating data ISR
ISR(TIMER1_COMPA_vect){
  sendBit = true;


  // increm++;
  // OCR2A = 0;

  // if ((bitstream_pos % 2) == 0){
  //     OCR2A = 50; // TODO
  //   } else{
  //     OCR2A = 1; // TODO
  //   }

  // bitstream_pos++;


  // Increment bitstream position
  // if (bitstream_pos >= string_len) {
  //   bitstream_pos = 0;
  //   // Disable interrupt if done transmitting
  //   TIMSK1 = 0;
  // } else {
  //   // Get current bit
  //   // int cur_bit = bitstream[bitstream_pos];
  //   int cur_bit = increm % 2;
  //   bitstream_pos++;

  //   // Change OCR2A? value if valid
  //   if (cur_bit == 1){
  //     OCR2A = 1; // TODO
  //   } else{
  //     OCR2A = 1; // TODO
  //   }

  // }
}


void loop() {
  if(sendingData){
    if(sendBit){
      // if ((bitstream_pos % 2) == 0){
      //   OCR2A = 2; // TODO
      // } else{
      //   OCR2A = 1; // TODO
      // }

      OCR2A = bitstream[bitstream_pos];

      sendBit = false;
      bitstream_pos++;

      if (bitstream_pos >= 7) {
        bitstream_pos = 0;
        // Disable interrupt if done transmitting
        TIMSK1 = 0;
        OCR2A = 0;
        sendingData = false;
      }
    }
  } else {
    Serial.println("Not sending data.");
    delay(500);
    startDataTransmission();
  }

}
// void loop() {

//   // Output sync waveform on startup
//   if (!synchronized){
//     Serial.println("Running Synchronization");

//     // Create sync waveform bitstream (1 - 0 repeating)
//     int test_bit = 1;
//     for(int i=0; i<maxCharLength; i++){
//       bitstream[i] = test_bit;
//       test_bit = (test_bit + 1) % 2;
//     }
//     Serial.println (bitstream[0]);
//     Serial.println (bitstream[1]);
//     Serial.println (bitstream[2]);
//     Serial.println (bitstream[3]);
//     Serial.println (bitstream[4]);
//     Serial.println (bitstream[5]);
//     Serial.println (bitstream[6]);
//     Serial.println (bitstream[7]);
//     Serial.println (bitstream[8]);




//     // Output the bitstream a set number of times
//     for(int i=0; i<numSyncTransmissions; i++){
//       startDataTransmission();
//     }
//     // synchronized = true;
//   }
//   // If synchronized, normal/user input operation
//   else
//   {
//     Serial.println("\nEnter data:");
//     // Wait for user to enter text in serial port
//     while (Serial.available() == 0) {
//       Serial.print("Increm: ");
//       Serial.println(increm);
//       delay(200);
//     }

//     data_string = Serial.readString(); // Read until timeout

//     // Remove any \r \n whitespace at the end of the String
//     data_string.trim();

//     // Verify string isn't too long
//     if (data_string.length() > maxCharLength){
//       Serial.print("Error: Data length exceeds maximum character length of ");
//       Serial.print(maxCharLength - 2);
//       Serial.println(". Try again.");
//     }
//     // Else if valid data, modulate and send the data string
//     else {
//       // Convert given string to bitstream representation
//       convertStringToBitStream(data_string, bitstream);
//       // sendDataSimple(data_string);
//       // sendData(bitstream);
//       // Start timer to output symbols at given rate
//       startDataTransmission();
//     }
//   }

//   // Wait for serial things to finish
//   delay(200);

// }