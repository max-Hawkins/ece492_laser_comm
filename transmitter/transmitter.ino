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

// Must be 1, 2, 4, or 8
#define BITS_PER_SYMBOL 1

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
uint8_t bitstream[maxCharLength * 8]; // Characters * bits per character
int num_symbols = 100;
uint8_t bitstream_pos = 0;
bool sendingData = false;
volatile bool sendBit = false;

void setup()
{
    pinMode(freqOutputPin, OUTPUT);
    long baudrate = 115200;
    Serial.begin(baudrate);
    Serial.println("--- Transmitter Program Startup ---\nBaud Rate: ");
    Serial.println(baudrate);
    Serial.println("\nBits per symbol: ");
    Serial.println(BITS_PER_SYMBOL);
    Serial.println("\n\nEnter text to send:");

    // Initialize dummy data for testing
    // bitstream[0] = OCR2AHIGH;
    // bitstream[1] = OCR2ALOW;
    // bitstream[2] = OCR2AHIGH;
    // bitstream[3] = OCR2ALOW;
    // bitstream[4] = OCR2AHIGH;
    // bitstream[5] = OCR2ALOW;
    // bitstream[6] = OCR2AHIGH;
    // bitstream[7] = OCR2ALOW;
    // bitstream[8] = OCR2AHIGH;

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

// Convert bit value to associated OCR2A value
int symbolToOCR(uint8_t symbol){
  if(BITS_PER_SYMBOL == 1){
    if(symbol){
        return OCR2AHIGH;
      }else{
        return OCR2ALOW;
      }
  } else if(BITS_PER_SYMBOL == 2){
      if(symbol==3){
        return OCR2AHIGH;
      }else if(symbol==2){
        return OCR2ALOW;
      }else if(symbol==1){
        return OCR2ALOW;
      }else if(symbol==0){
        return OCR2ALOW;
      }
  } else if(BITS_PER_SYMBOL == 4){
      if(symbol==15){
        return OCR2AHIGH;
      }else if(symbol==14){
        return OCR2ALOW;
      }else if(symbol==13){
        return OCR2ALOW;
      }else if(symbol==12){
        return OCR2ALOW;
      }else if(symbol==11){
        return OCR2ALOW;
      }else if(symbol==10){
        return OCR2ALOW;
      }else if(symbol==9){
        return OCR2ALOW;
      }else if(symbol==8){
        return OCR2ALOW;
      }else if(symbol==7){
        return OCR2ALOW;
      }else if(symbol==6){
        return OCR2ALOW;
      }else if(symbol==5){
        return OCR2ALOW;
      }else if(symbol==4){
        return OCR2ALOW;
      }else if(symbol==3){
        return OCR2ALOW;
      }else if(symbol==2){
        return OCR2ALOW;
      }else if(symbol==1){
        return OCR2ALOW;
      }else if(symbol==0){
        return OCR2ALOW;
      }
  } else if(BITS_PER_SYMBOL == 8){
    // TODO: If only C had better metaprogramming capabilities :(
  }
}

// Insert the OCR2A values corresponding to the binary representation of character c into
// the bitstream at given pointer/index
void insertCharBits(char c, volatile uint8_t * bitstream){
  // Iterate over symbols in a character
  for(uint8_t j = 0; j < (8 / BITS_PER_SYMBOL); j++) {
    // Calculate index into bitstream
    long bitstream_idx = ((8 / BITS_PER_SYMBOL) - 1 - j);
    // Bit index into character of current symbol
    int char_bit_idx = j * BITS_PER_SYMBOL;

    uint8_t cur_symbol = 0;
    // Create symbol to convert to OCR2A
    // Iterates for number of bits in a symbol
    for(uint8_t symbol_i=0; symbol_i<BITS_PER_SYMBOL; symbol_i++){
      // Store current bit value
      uint8_t cur_bit_value = (c & (1 << (char_bit_idx + symbol_i)));
      cur_symbol += cur_bit_value;

      Serial.print("J: ");
      Serial.print(j);
      Serial.print("  char_bit_idx: ");
      Serial.print(char_bit_idx);
      Serial.print(" symbol_i: ");
      Serial.print(symbol_i);
      Serial.print("  cur_bit_value: ");
      Serial.print(cur_bit_value);
      Serial.print("  cur_symbol: ");
      Serial.println(cur_symbol);
    }
    Serial.print("\n---Current symbol: ");
    Serial.println(cur_symbol);
    bitstream[j] = symbolToOCR(cur_symbol);
  }
}

// Convert a given string to its binary format in an array of bools
void convertStringToBitStream(String _data_string, volatile uint8_t * bitstream){
  Serial.print("Converting \"");
  Serial.print(_data_string);
  Serial.println("\" to bitstream");
  Serial.print("Num Characters: ");
  Serial.print(_data_string.length());

  char cur_char;
  // Iterate through the characters in the string
  for(int i =0; i < _data_string.length(); i++ ) {
    cur_char = _data_string[i];
    Serial.print("\nChar: ");
    Serial.println(cur_char);
    Serial.print("Binary (lsb to msb): ");

    // Append current character's bits to bit representation
    insertCharBits(cur_char, &bitstream[i*8/BITS_PER_SYMBOL]);
  }
  Serial.println();
  // Append end of data char (newline)
  // insertCharBits(stopChar, &bitstream[(_data_string.length() + 1) * 8]);
  bitstream[(_data_string.length() + 1) * (8 / BITS_PER_SYMBOL) + 1] = 2; // End of valid data flag
  // Calculate number of symbols in this data transmission
  // Number of characters * 8 bits per character / BITS_PER_SYMBOL
  num_symbols = _data_string.length() * (8 / BITS_PER_SYMBOL);
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
}

// Updating data ISR
ISR(TIMER1_COMPA_vect){
  sendBit = true;
}

void loop() {
  if(sendingData){
    if(sendBit){
      if (bitstream_pos >= num_symbols) {
        bitstream_pos = 0;
        // Disable interrupt if done transmitting
        TIMSK1 = 0;
        OCR2A = OCR2AREST;
        sendingData = false;
      } else {
        // Set output compare register A for timer 2 to desired value
        OCR2A = bitstream[bitstream_pos];
        // Reset sendBit flag for ISR
        sendBit = false;
        // Serial.print("bitstream pos: ");
        // Serial.println(bitstream_pos);
        // Serial.print("num_symbols: ");
        // Serial.println(num_symbols);

        // Increment the position in the array that stores desired OCR2A values
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
      // for(int i=0; i<num_symbols; i++){
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