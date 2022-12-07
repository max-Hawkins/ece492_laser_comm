// Receiver/Frequency Detection

// This code measures the frequency of a given input signal (preferably square-wave,
// but sinusoid works as well) at a given update rate. It does this by treating the
// input waveform as the clock input for counter 1. Then clock 2 is used to create
// an ISR with a variable period (the given update rate) to determine the number
// of "clock" (input signal) events since the last clock 2 ISR. By keeping track of
// how often the clock 2 ISR is called and the base clock frequency of the Arduino,
// the frequency of the input signal can be estimated.
// See page 102 of the Arduino Nano datasheet for the diagram of the counter signal path

// Max Frequency detection of ~8MHz
// Can handle sinusoid but is a bit more noisy
// Input pin: D5

// Original Author: Nick Gammon
// Date: 17th January 2012
// https://arduino.stackexchange.com/questions/55028/read-frequency-of-input-in-digital-pins
// Updated by Max Hawkins
// Date: Fall 2022
//----------------------------------------------------------------------------

// Must be 1, 2, 3, or 4
#define BITS_PER_SYMBOL 1
// Print Debug Info
#define VERBOSE false

// Timer-tracking variables
volatile unsigned long timerCounts;
const int maxCharacters = 40;
const int timerCountsSize = maxCharacters * 8 * 4; // Max characters * 8 bits * 4 samples/bit
// Cutoff frequency (in kHz) delineating valid and rest data in MHz
const int validDataFreqCutoff = 126;
int timerCountsIdx = 0;
uint8_t timerCountsArray[timerCountsSize];
volatile boolean counterReady;
volatile boolean prevValidData, curValidData;
unsigned long overflowCount;
unsigned int timerTicks;
unsigned int timerPeriod;
int timerCountsValidCutoff;

// Start counting the number of rising edges into Timer 1 every 'ms' milliseconds
void startCounting (unsigned int ms){
  counterReady = false; // time not up yet
  timerPeriod = ms;      // how many 1 ms counts to do
  timerCountsValidCutoff = validDataFreqCutoff * ms;
  timerTicks = 0;       // reset interrupt counter
  overflowCount = 0;    // no overflows yet

  // Reset Timer 1 and Timer 2
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR2A = 0;
  TCCR2B = 0;

  // Timer 1 - counts events on pin D5
  TIMSK1 = bit (TOIE1);   // Interrupt on Timer 1 overflow

  // Timer 2 - gives us our 1 ms counting interval
  // TODO: Alter prescaling?
  // 16 MHz clock (62.5 ns per tick) - prescaled by 128
  // counter increments every 8 µs.
  // So we count 125 of them, giving exactly 1000 µs (1 ms)
  TCCR2A = bit (WGM21) ;   // 21CTC mode
  // TODO: Change this
  // Set output compare register to the value to count up to (zero relative!)
  OCR2A  = 124;

  // Set the Timer2 Compare Match A Interrupt Enable pin
  // to enable the interrupt at the desired frequency
  TIMSK2 = bit (OCIE2A);

  // Set both counter 1 and 2 to zero
  TCNT1 = 0;
  TCNT2 = 0;

  // "Setting the PSRASY bit in GTCCR resets the prescaler.
  //  This allows the user to operate with a predictable prescaler." - datasheet
  GTCCR = bit (PSRASY);

  // Start Timer 2 with a prescaler of 128
  // See page 165 for details on changing this
  TCCR2B =  bit (CS20) | bit (CS22) ;

  // Start Timer 1 with an external clock source from T1 pin (D5).
  // Clock on rising edge.
  TCCR1B =  bit (CS10) | bit (CS11) | bit (CS12);
  }  // End of startCounting

ISR (TIMER1_OVF_vect){
  ++overflowCount; // Increment number of Counter1 overflows
}  // End of TIMER1_OVF_vect


//******************************************************************
//  Timer2 Interrupt Service is invoked by hardware Timer 2 every 1 ms = 1000 Hz
//  Timer2 ISR Frequency = Base Clock Frequency / Timer2 Prescaler / (OCR2A + 1)
//  16Mhz / 128 / 125 = 1000 Hz
ISR (TIMER2_COMPA_vect){
  // Grab counter value before it changes anymore
  // TODO: Make this volatile? Does the initialization of this take time?
  // How are ISR's compiled?
  // See pages 117 and 143 for more info on this and accessing 16-bit registers
  unsigned int timer1CounterValue;
  timer1CounterValue = TCNT1;

  // Grab overflow count before it changes anymore
  unsigned long overflowCopy = overflowCount;

  // Exit early if we haven't reached timing period
  // This accounts for multiples of milliseconds counted
  if (++timerTicks < timerPeriod)
    return;

  // If just missed an overflow, increment the overlow counter
  if ((TIFR1 & bit (TOV1)) && timer1CounterValue < 256)
    overflowCopy++;

  // End of gate time, measurement ready

  // Stop Timer 1
  TCCR1A = 0;
  TCCR1B = 0;

  // Stop Timer 2
  TCCR2A = 0;
  TCCR2B = 0;

  // Disable Timer 1 and 2 interrupts
  TIMSK1 = 0;
  TIMSK2 = 0;

  // Calculate total counts accounting for overflows
  // Each overflow is 65536 more
  timerCounts = (overflowCopy << 16) + timer1CounterValue;

  // Set global flag for end count period
  // TODO: To make continuous operation, remove this and handle appropriately
  counterReady = true;

  // If the number of counts is less than 500, this indicates start of valid data
  // Store previous valid data state
  prevValidData = curValidData;
  // Update current valid data state
  curValidData = (timerCounts > timerCountsValidCutoff) ? 0 : 1;
}  // End of TIMER2_COMPA_vect

// Return frequency in MHz corresponding to the number of counts
// TODO: If make counting period longer, alter this calculation - redundant right now
float countsToFreq(unsigned long timerCounts){
  return (timerCounts *  1000.0) / (timerPeriod * 1000);
}

// Return symbol value corresponding to frequency
uint8_t freqToSymbol(float freq){
  if(BITS_PER_SYMBOL == 1){
    if(freq > 112){
        return 1;
      }else{
        return 0;
      }
  } else if(BITS_PER_SYMBOL == 2){
      if(freq > 120){
        return 3;
      }else if(freq > 115){
        return 2;
      }else if(freq > 105){
        return 1;
      }else{
        return 0;
      }
  } else if(BITS_PER_SYMBOL == 3){
      if(freq > 121){
        return 7;
      }else if(freq > 117){
        return 6;
      }else if(freq > 113){
        return 5;
      }else if(freq > 109){
        return 4;
      }else if(freq > 106){
        return 3;
      }else if(freq > 104){
        return 2;
      }else if(freq > 101){
        return 1;
      }else{
        return 0;
      }
  } else if(BITS_PER_SYMBOL == 4){
      if(freq > 122){
        return 15;
      }else if(freq > 120){
        return 14;
      }else if(freq > 118){
        return 13;
      }else if(freq > 117){
        return 12;
      }else if(freq > 115){
        return 11;
      }else if(freq > 113){
        return 10;
      }else if(freq > 112){
        return 9;
      }else if(freq > 110){
        return 8;
      }else if(freq > 109){
        return 7;
      }else if(freq > 107){
        return 6;
      }else if(freq > 105){
        return 5;
      }else if(freq > 104){
        return 4;
      }else if(freq > 103){
        return 3;
      }else if(freq > 101){
        return 2;
      }else if(freq > 100){
        return 1;
      }else{
        return 0;
      }
  }
}

// Print counts, frequency, and associated bit value of a number of rising edges
void printCountsData(int counts){
  Serial.print("Rising Edges: ");
  Serial.print(counts);
  Serial.print("\tFreq: ");
  float freq = countsToFreq(counts);
  Serial.print(freq);
  Serial.print(" kHz\tSymbol: ");
  Serial.println(freqToSymbol(freq));
}

// Remove 'exterior' samples and average interior samples to create valid data
int removeTransitions(int endDataIdx){
  // If missing just one "bit", treat it as good data since just dropped last transition to rest
  if((endDataIdx + 1) % 4 == 3){
    if(VERBOSE) Serial.println("Just missed last transition!");
    endDataIdx++;
  }
  // Number of bits in the data packet
  int dataSymbolSize = (endDataIdx + 1) / 4;
  int dataBitSize = dataSymbolSize * BITS_PER_SYMBOL;

  if(VERBOSE){
    Serial.println("Remove transitions:");
    Serial.print("Received data bit size: ");
    Serial.println(dataBitSize);
  }

  bool dataBitArray[dataBitSize];

  for(int i=1; i<=endDataIdx; i+=4){
    if(VERBOSE){
      Serial.println(i);
      printCountsData(timerCountsArray[i]);
      printCountsData(timerCountsArray[i+1]);
      printCountsData(timerCountsArray[i+2]);
      printCountsData(timerCountsArray[i+3]);
    }
    // Average "inner" two samples of oversampled input
    int averageCounts = (timerCountsArray[i] + timerCountsArray[i+1]) / 2;
    uint8_t symbol = freqToSymbol(countsToFreq(averageCounts));
    if(VERBOSE){
      Serial.print("Symbol: ");
      Serial.println(symbol);
    }
    // Iterate through symbol bits and add to output dataBitArray
    for(int symbol_i=0; symbol_i<BITS_PER_SYMBOL; symbol_i++){
      // Get current bit in symbol
      bool cur_bit = symbol & (1 << symbol_i); // TODO
      // Put current bit into output data array
      int dataBitArrayIdx = (i - 1) / 4; // TODO

      Serial.print("Symbol_i: ");
      Serial.print(symbol_i);
      Serial.print("  cur_bit: ");
      Serial.print(cur_bit);
      Serial.print("  dataBitArrayIdx: ");
      Serial.println(dataBitArrayIdx);

      dataBitArray[dataBitArrayIdx] = cur_bit;
    }
    // Replace timer counts data with average counts
    timerCountsArray[(i - 1) / 4] = averageCounts;
  }

  Serial.println("\nProcessed data:");
  for(int i=0; i<dataBitSize; i++){
    Serial.print(dataBitArray[i]);
    if(i != 0 && i % 8 == 0){
      Serial.println();
    }
  }
  Serial.println();

  return dataBitSize - 1;
}

// Process valid data once it's been fully received
void processValidData(int endDataIdx){
  // Number of bits in the data packet
  int dataBitSize = endDataIdx + 1;
  // Data size padded to next smallest multiple of 8
  int padDataBitSize = ((dataBitSize + 7) >> 3) << 3;
  // Padded data size in bytes
  int padDataByteSize = padDataBitSize / 8;
  // Bitstream storage
  bool padDataBitArray[padDataBitSize];
  // Byte storage
  byte padDataByteArray[padDataByteSize];

  // Zero out padDataByteArray TEST
  Serial.println("---bytes:");
  for(int i=0; i<padDataByteSize; i++){
    padDataByteArray[i] = 0;
  }
  // Zero out padDataByteArray TEST
  Serial.println("---bytes:");
  for(int i=0; i<padDataByteSize; i++){
    Serial.println(padDataByteArray[i]);
  }

  if (VERBOSE){
    Serial.print("Pad bit size: ");
    Serial.println(padDataBitSize);

    Serial.print("Pad byte size: ");
    Serial.println(padDataByteSize);

    Serial.print("Received data of bit length: ");
    Serial.println(dataBitSize);

    // Print every count, frequency, and bit
    Serial.println("Each bit's detailed data:");
    for(int idx=0; idx<=endDataIdx; idx++){
      printCountsData(timerCountsArray[idx]);
    }

    Serial.println("\nBit representation:");
  }

  // Zero out padDataByteArray TEST
  Serial.println("---bytes:");
  for(int i=0; i<padDataByteSize; i++){
    Serial.println(padDataByteArray[i]);
  }
  // Print bit values in byte groups
  for(int idx=0; idx<padDataBitSize; idx+=BITS_PER_SYMBOL){
    // Padded symbols are 0
    uint8_t symbol = 0;

    if(idx <= endDataIdx){
      symbol = freqToSymbol(countsToFreq(timerCountsArray[idx]));
    }
    // Insert bits into bitstream array
    for(int symbol_i=0; symbol_i<BITS_PER_SYMBOL; symbol_i++){
      // Get current bit in symbol
      bool cur_bit = symbol & (1 << symbol_i); // TODO
      padDataBitArray[idx + symbol_i] = cur_bit;


      if(VERBOSE){
        // If beginning of new byte, print on newline
        if(idx != 0 && idx % 8 == 0){
          Serial.println();
        }

        Serial.print(cur_bit);
      }

      // Create byte array
      int i = idx % 8;
      int byteIdx = floor(idx / 8);
      if(cur_bit){
        padDataByteArray[byteIdx] |= (1 << (7-i));
      }
      if (VERBOSE){
        // If last bit in the current byte, display byte as ASCII
        if(i == 7){
          Serial.print(" - ");
          Serial.print(byteIdx);
          Serial.print(" - ");
          Serial.print(padDataByteArray[byteIdx]);
          Serial.print(" - ");
          Serial.write(padDataByteArray[byteIdx]);
        }
      }
    }
  }

  // Zero out padDataByteArray TEST
  Serial.println("---bytes:");
  for(int i=0; i<padDataByteSize; i++){
    Serial.println(padDataByteArray[i]);
  }

  if(VERBOSE)
    Serial.println();

  // Write bytes to serial port to view ASCII representation
  Serial.println("\nASCII Encoding:");
  Serial.write(padDataByteArray, padDataByteSize);
  Serial.println();

  Serial.println("\nDone processing data. Waiting for more input...\n");
}

// Program startup - print configuration details to user
void setup (){
  long baudrate = 115200;
  Serial.begin(baudrate);
  Serial.print("--- Receiver Program Startup --- \nBaud Rate: ");
  Serial.println(baudrate);
  Serial.print("Bits per symbol: ");
  Serial.println(BITS_PER_SYMBOL);
  Serial.println("Waiting for data transmission...\n");

  prevValidData = false; // Waiting for frequency trigger to indicate data start
  curValidData  = false;
} // End of setup

void loop (){
  // Stop Timer 0 interrupts from throwing the count out
  byte oldTCCR0A = TCCR0A;
  byte oldTCCR0B = TCCR0B;

  // Stop Timer 0
  TCCR0A = 0;
  TCCR0B = 0;

  startCounting (1);  // Count number of rising edges every X milliseconds

  while (!counterReady){} // Busy loop until count over

  // Uncomment for continuous reporting of frequency detected
  // printCountsData(timerCounts);

  // If the current bit is valid
  if(curValidData){
    // Serial.println("Valid Data!------------------------");
    if(!prevValidData){
      // Serial.println("\n\nStarting to read data...");
      timerCountsIdx = 0; // Reset timer counts index on valid data start
    }
    // Add current timerCounts to array
    timerCountsArray[timerCountsIdx] = timerCounts;

    // Increment index with rollover
    // Shouldn't need rollover but better than out of bounds access
    timerCountsIdx = (timerCountsIdx + 1) % timerCountsSize;
  } else {
    // If done getting valid data
    if(prevValidData){
      // Process valid data acquired
      if(VERBOSE)
        processValidData(timerCountsIdx - 1);
      // Remove exterior samples of oversampled input frequencies
      int newBitSize = removeTransitions(timerCountsIdx - 1);
      // Process good data
      processValidData(newBitSize);
    }
  }

  // Restart timer 0
  TCCR0A = oldTCCR0A;
  TCCR0B = oldTCCR0B;
}   // End of loop
