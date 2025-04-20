void setup() {
  DDRA = 0b00000001; //pin 22 output
  PORTA = 0b00000001; //pin 22 default low


  //Timer 1 Setup to trigger at specified interval 
  TCCR1A = 0b00000000;  // Normal port operation, CTC mode
  TCCR1B = 0b00001010;  // WGM1[3:2] = 10 (CTC mode), CS1[2:0] = 010 (prescaler 8), sets ctc mode on, resets timer to 0 after reach certain time
  TCCR1C = 0b00000000;

  OCR1A = 499; 

  TIMSK1 = 0b00000010; //sets for interrupt to trigger based on the timer 
}

void loop() {
  // put your main code here, to run repeatedly:

}


//Timer Interrupt to set the stepper motor signal at a specified interval 
ISR(TIMER1_COMPA_vect) 
{
  PINA = 0b00000011; // Toggle PA0 (Pin 22) and PA1 (Pin 23) only when moving
}