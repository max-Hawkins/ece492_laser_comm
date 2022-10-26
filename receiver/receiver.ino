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


// Timer-tracking variables
volatile unsigned long timerCounts;
const int timerCountsSize = 100;
const int validDataFreqCutoff = 700; // Cutoff frequency delineating valid and rest data in MHz
int timerCountsIdx = 0;
unsigned long timerCountsArray[timerCountsSize];
volatile boolean counterReady;
volatile boolean prevValidData, curValidData;
unsigned long overflowCount;
unsigned int timerTicks;
unsigned int timerPeriod;
int timerCountsValidCutoff;

// TODO: Move this to an ISR as well?
void startCounting (unsigned int ms)
  {
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
ISR (TIMER2_COMPA_vect)
  {
  // Grab counter value before it changes anymore
  // TODO: Make this volatile? Does the initialization of this take time?
  // How are ISR's compiled/handled?
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
  TCCR1A = 0;    // stop timer 1
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
float countsToFreq(unsigned long timerCounts){
  return (timerCounts *  1000.0) / (timerPeriod * 1000);
}

// Return bit value corresponding to frequency
int freqToBit(float freq){
  if(freq > 140){
    return 1;
  }
  else{
    return 0;
  }
}

// Print counts, frequency, and associated bit value of a number of rising edges
void printCountsData(int counts){
  Serial.print("Rising Edges: ");
  Serial.print(counts);
  Serial.print("\tFreq: ");
  float freq = countsToFreq(counts);
  Serial.print(freq);
  Serial.print(" MHz\tBit: ");
  Serial.println(freqToBit(freq));
}

void processValidData(int endDataIdx){
  Serial.print("Received data of bit length: ");
  Serial.println(endDataIdx);

  // Print every count, frequency, and bit
  for(int idx=0; idx<=endDataIdx; idx++){
    printCountsData(timerCountsArray[idx]);
  }

  // Print bit values in byte groups
  for(int idx=0; idx<=endDataIdx; idx++){
    int bit = freqToBit(countsToFreq(timerCountsArray[idx]));
    // If beginning of new byte
    if(idx != 0 && idx % 8 == 0){
      Serial.println();
    }
    Serial.print(bit);
  }
  Serial.println();

  Serial.println("ASCII Encoding:");

  // TODO: Convert bitstream to bytes and write to serial

  boolean array[8] = {false, false, true, false, false, false, false, true};

  byte result = 0;

  for(int i=0; i<8; i++){
      if(array[i]){
        result |= (1 << (7-i));
      }
  }

  Serial.write(result);
  Serial.println();

  Serial.println("\nDone processing data. Waiting for more input...\n");
}

void setup ()
  {
  Serial.begin(115200);
  Serial.println("Receiver Program Startup");
  Serial.println("Waiting for data transmission...");

  prevValidData = false; // Waiting for frequency trigger to indicate data start
  curValidData  = false;
  } // End of setup

void loop ()
  {
  // Stop Timer 0 interrupts from throwing the count out
  byte oldTCCR0A = TCCR0A;
  byte oldTCCR0B = TCCR0B;

  // Stop Timer 0
  TCCR0A = 0;
  TCCR0B = 0;

  startCounting (1);  // Count number of rising edges every X milliseconds

  while (!counterReady){} // Busy loop until count over

  // printCountsData(timerCounts);
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
      processValidData(timerCountsIdx - 1);
    }
  }

  // Restart timer 0
  TCCR0A = oldTCCR0A;
  TCCR0B = oldTCCR0B;
  }   // End of loop
