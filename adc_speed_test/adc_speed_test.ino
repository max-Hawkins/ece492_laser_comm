int adc_reso = 16;
long num_reads_per_loop = 100000000000;

void setup() {
  Serial.begin(9600);
  Serial.print("Hello");
//  analogReadResolution(adc_reso);
}

void loop() {

  // Timing loop
  long tic = millis();
  double adc_val;
  for(int i=0; i++; i<num_reads_per_loop){
    adc_val = analogRead(3);
  }
  long toc = millis();
  double adc_read_time = (toc - tic) / num_reads_per_loop;

  // Print time
  Serial.print("ADC Resolution:");
  Serial.println(adc_reso);
  Serial.print("Time per analog read: ");
  Serial.println(adc_read_time);
  Serial.println(tic);
  Serial.println(toc);

  
}
