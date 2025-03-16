volatile uint16_t t_on = 0;  // Stores high-time (PWM on time)
volatile uint16_t t_off = 0; // Stores low-time (PWM off time)
volatile bool newData = false;

void setup() 
{
  cli(); //ensures that interrupts don't interfere this block of code

  Serial.begin(115200);

  DDRA = 0b00000011;  //sets both pins 22 and 23 as output pins
  PORTA = 0b00000000; //sets both pins 22 and 23 to start at low

  DDRD  = 0b00000000;
  EICRA = 0b00001111;
  EIMSK = 0b00000011;


  TCCR1A = 0b00000000;  // Normal port operation, CTC mode
  TCCR1B = 0b00001010;  // WGM1[3:2] = 10 (CTC mode), CS1[2:0] = 010 (prescaler 8), sets ctc mode on, resets timer to 0 after reach certain time
  TCCR1C = 0b00000000;

  OCR1A = 999; //compare match value of 100 micro sec (e.g switches between H/L every 100micro or with period 200micro)
  // eqn to calculate above number: (timer clock * time interval / prescaler) - 1

  TIMSK1 = 0b00000010; //sets for interrupt to trigger based on the timer 
  
  DDRL &= ~(1 << PL1); //Sets pin 48 as an input

  TCCR5A = 0b00000000;  // Normal mode
  TCCR5B = 0b11000010;  // Input capture on falling edge, Prescaler = 8 (2MHz clock)
  TCCR5C = 0b00000000;

  TIMSK5 = 0b00100001;  // Enable Input Capture and Overflow Interrupts

  sei();
}

void loop() 
{
  encoderposition();  // Continuously update encoder position

  delay(100);
}

//Timer Interrupt to set the stepper motor signal at a specified interval 
ISR(TIMER1_COMPA_vect) 
{
  PINA = 0b00000011; // Toggle PA0 (Pin 22) and PA1 (Pin 23) between H/L
}

//Timer interrupt to trigger once a falling or rising edge is detected 
ISR(TIMER5_CAPT_vect) 
{
  static uint16_t lastCapture = 0;
  static bool measuringOnTime = true;
  uint16_t currentCapture = ICR5;  // Read Timer5 capture register

  if (measuringOnTime) 
  {
    t_on = currentCapture - lastCapture;  // Measure high time
    TCCR5B ^= (1 << ICES5);  // Toggle to capture rising edge next
  } 
  else 
  {
    t_off = currentCapture - lastCapture; // Measure low time
    newData = true; // New data ready
    TCCR5B ^= (1 << ICES5);  // Toggle to capture falling edge next
  }

  measuringOnTime = !measuringOnTime; 
  lastCapture = currentCapture;
}

void encoderposition()
{
  if (newData) 
  {
    cli();
    uint16_t highTime = t_on;
    uint16_t lowTime = t_off;
    newData = false;
    sei();

    float t_on_us = highTime * 0.5;
    float t_off_us = lowTime * 0.5;

    // Compute absolute position
    float x = ((t_on_us * 1026) / (t_on_us + t_off_us)) - 1;
    uint16_t position = (x <= 1022) ? x : 1023;

    // Send data to Raspberry Pi over serial
    Serial.print(highTime);
    Serial.print(",");
    Serial.print(lowTime);
    Serial.print(",");
    Serial.print(position);
    Serial.println();
    Serial.flush();
  }
}
