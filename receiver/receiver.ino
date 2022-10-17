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
int timerCountsIdx = 0;
unsigned long timerCountsArray[timerCountsSize];
volatile boolean counterReady;
unsigned long overflowCount;
unsigned int timerTicks;
unsigned int timerPeriod;

// Timing Metrics
// TODO: Implement
unsigned long tic;
unsigned long toc;

// TODO: Move this to an ISR as well
void startCounting (unsigned int ms)
  {
  counterReady = false; // time not up yet
  timerPeriod = 0;     // how many 1 ms counts to do
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
  //  counter increments every 8 µs.
  // So we count 125 of them, giving exactly 1000 µs (1 ms)
  TCCR2A = bit (WGM21) ;   // 21CTC mode
  // TODO: Change this
  // Set output compare register to the value to count up to (zero relative!)
  // OCR2A  = 124;
  OCR2A

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

ISR (TIMER1_OVF_vect)
  {
    // Increment number of Counter1 overflows
    ++overflowCount;
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
  if (++timerTicks < timerPeriod)
    return;

  // If just missed an overflow, increment the overlow counter
  if ((TIFR1 & bit (TOV1)) && timer1CounterValue < 256)
    overflowCopy++;

  // end of gate time, measurement ready

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
  }  // End of TIMER2_COMPA_vect

float countsToFreq(unsigned long timerCounts){
  return (timerCounts *  1000.0) / timerPeriod;
}

int freqToBit(float freq){
  if(freq > 2500000){
    return 1;
  }
  else{
    return 0;
  }
}

void setup ()
  {
  Serial.begin(115200);
  Serial.println("Receiver Program Startup\n");

  } // End of setup

void loop ()
  {
  // Stop Timer 0 interrupts from throwing the count out
  byte oldTCCR0A = TCCR0A;
  byte oldTCCR0B = TCCR0B;

  // Stop Timer 0
  TCCR0A = 0;
  TCCR0B = 0;

  // TODO: Change input to be in
  startCounting (1);  // how many ms to count for

  while (!counterReady){} // Busy loop until count over

  // Add current timerCounts to array
  timerCountsArray[timerCountsIdx] = timerCounts;

  // If just filled timerCountsArray, display array
  if (timerCountsIdx == timerCountsSize - 1){
    Serial.println ("---------- Printing Collected data!!! -------");
    unsigned long counts;
    float _freq;
    int _data;
    Serial.println (timerCountsArray[0]);
    Serial.println (timerCountsArray[1]);
    Serial.println (timerCountsArray[2]);
    Serial.println (timerCountsArray[3]);
    Serial.println (timerCountsArray[4]);
    Serial.println (timerCountsArray[5]);
    Serial.println (timerCountsArray[6]);
    Serial.println (timerCountsArray[7]);
    Serial.println (timerCountsArray[8]);
    Serial.println (timerCountsArray[9]);
    Serial.println (timerCountsArray[10]);
    Serial.println (timerCountsArray[11]);
    Serial.println (timerCountsArray[12]);
    Serial.println (timerCountsArray[13]);
    Serial.println (timerCountsArray[14]);
    Serial.println (timerCountsArray[15]);
    Serial.println (timerCountsArray[16]);
    Serial.println (timerCountsArray[17]);
    Serial.println (timerCountsArray[18]);
    Serial.println (timerCountsArray[19]);
    Serial.println (timerCountsArray[20]);
    Serial.println (timerCountsArray[21]);
    Serial.println (timerCountsArray[22]);
    Serial.println (timerCountsArray[23]);
    Serial.println (timerCountsArray[24]);
    Serial.println (timerCountsArray[25]);
    Serial.println (timerCountsArray[26]);
    Serial.println (timerCountsArray[27]);
    Serial.println (timerCountsArray[28]);
    Serial.println (timerCountsArray[29]);
    Serial.println (timerCountsArray[30]);
    Serial.println (timerCountsArray[31]);
    Serial.println (timerCountsArray[32]);
    Serial.println (timerCountsArray[33]);
    Serial.println (timerCountsArray[34]);
    Serial.println (timerCountsArray[35]);
    Serial.println (timerCountsArray[36]);
    Serial.println (timerCountsArray[37]);
    Serial.println (timerCountsArray[38]);
    Serial.println (timerCountsArray[39]);
    Serial.print (countsToFreq(timerCountsArray[39]));
    Serial.println(" Hz");

    // delay(1000);
    // for(int idx=0; idx++; idx<2){
    //   Serial.print ((int) idx);

      // counts = timerCountsArray[idx];
      // Serial.print ("\nCounts: ");
      // Serial.print (counts);

      // _freq = countsToFreq(counts);
      // _data = freqToBit(_freq);


      // Serial.print ("  Frequency: ");
      // Serial.print ( _freq);
      // Serial.print (" Hz.");
      // Serial.print ("  Data: ");
      // Serial.println (_data);
      // delay(200);
    // }
    // delay(1000);
  }
  // Increment index with rollover
  timerCountsIdx = (timerCountsIdx + 1) % timerCountsSize;

  // Adjust counts by counting interval to give frequency in Hz
  // TODO: Change 1000 in accordance with above
  // float frq = (timerCounts *  1000.0) / timerPeriod;
  // int data;

  // // Dead simple modulation scheme
  // if(frq > 2500000){
  //   data = 1;
  // }
  // else{
  //   data = 0;
  // }

  // Serial.print ("\nFrequency: ");
  // Serial.print ((unsigned long) frq);
  // Serial.println (" Hz.");
  // Serial.print ("Data: ");
  // Serial.println (data);

  // Restart timer 0
  TCCR0A = oldTCCR0A;
  TCCR0B = oldTCCR0B;



  // Wait for serial stuff to finish
  // delay(1000);
  }   // End of loop
