#define CLR(x,y) (x&=(~(1<<y)))

#define SET(x,y) (x|=(1<<y))

#define _BV(bit) (1 << (bit))

#define pin 1

void setup() {
  pinMode(pin, OUTPUT);
}

void loop() {
  // Speed: 148kHz
  digitalWrite(pin, LOW);
  digitalWrite(pin, HIGH);
  
//  CLR(PORTB, 0) ;
//  SET(PORTB, 0);
  
//  PORTB |= _BV(0);                   
//  PORTB &= ~(_BV(0));
}
