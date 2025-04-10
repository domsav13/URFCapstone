volatile uint16_t tilt_t_on = 0;  // Stores high-time (PWM on time)
volatile uint16_t tilt_t_off = 0; // Stores low-time (PWM off time)
volatile bool tiltNewData = false;

volatile uint16_t tiltTargetPosition = 400; 
volatile uint16_t tiltPosition = 0;
volatile bool tiltMotorRun = false;

volatile uint16_t pan_t_on = 0;  // Stores high-time (PWM on time)
volatile uint16_t pan_t_off = 0; // Stores low-time (PWM off time)
volatile bool panNewData = false;

volatile uint16_t panTargetPosition = 400; 
volatile uint16_t panPosition = 0;
volatile bool panMotorRun = false;

void setup() 
{
  cli(); //ensures that interrupts don't interfere this block of code

  //Pins 22, 24, 48 (Timer 5), Timer 1 go to the tilt motor
  //Pins 23, 25, 7 (Timer 4), Timer 3 go to the pan motor


  //Setup of Clock and direction singals for both the tilt and pan motors
  DDRA = 0b00001111;  //sets pins 22,23,24,25 as output pins 
  PORTA = 0b00000000; //sets pins 22,23,24,25 to start at low

  //TILT MOTOR
  //Timer 1 Setup for tilt motor signal 
  DDRD  = 0b00000000;
  EICRA = 0b00001111;
  EIMSK = 0b00000011;

  TCCR1A = 0b00000000;  // Normal port operation, CTC mode
  TCCR1B = 0b00001010;  // WGM1[3:2] = 10 (CTC mode), CS1[2:0] = 010 (prescaler 8), sets ctc mode on, resets timer to 0 after reach certain time
  TCCR1C = 0b00000000;

  OCR1A = 999; //compare match value of 100 micro sec (e.g switches between H/L every 100micro or with period 200micro)
  // eqn to calculate above number: (timer clock * signal period / 2 * prescaler) - 1

  TIMSK1 = 0b00000010; //sets for interrupt to trigger based on the timer 
  
  //Timer 5 Setup for capturing tilt encoder singal 
  DDRL &= ~(1 << PL1); //Sets pin 48 as an input for the tilt encoder

  TCCR5A = 0b00000000;  
  TCCR5B = 0b11000010;  // Input capture on falling edge, Prescaler = 8 (2MHz clock)
  TCCR5C = 0b00000000;

  TIMSK5 = 0b00100001;  // Enable Input Capture and Overflow Interrupts

  //PAN MOTOR
  //Timer 3 setup for pan motor singal 
  TCCR3A = 0b00000000;  // Normal port operation, CTC mode
  TCCR3B = 0b00001010;  // WGM3[3:2] = 10 (CTC mode), CS3[2:0] = 010 (prescaler 8)
  TCCR3C = 0b00000000;

  OCR3A = 1599; //Adjust this for pan motor speed

  //Timer 4 Setup for capturing pan encoder signal 
  
  DDRH &= ~(1 << PH7); // Sets pin 7 as an input for the encoder

  TCCR4A = 0b00000000;  
  TCCR4B = 0b11000010;  // Input capture on falling edge, Prescaler = 8 (2MHz clock)
  TCCR4C = 0b00000000;

  TIMSK4 = 0b00100001;  // Enable Input Capture and Overflow Interrupts
  sei();

}

void loop() 
{
  tiltEncoderposition();  // Continuously update encoder position

  if ((int16_t)tiltPosition - (int16_t)tiltTargetPosition < 512)
  {
    PORTA &= ~(1 << 2);
  }
  else
  {
    PORTA |= (1 << 2);
  }

  if (tiltMotorRun && abs((int16_t)tiltPosition - (int16_t)tiltTargetPosition) <= 50) 
  {
    tiltMotorRun = false;  // Stop the motor
  }

  if (!tiltMotorRun && abs((int16_t)tiltPosition - (int16_t)tiltTargetPosition) >= 60) 
  {
    tiltMotorRun = true;   // Restart the motor
  }

  if (tiltMotorRun) //do this to completely disable timer when motor running is false instead of checking its values during every interrupt
  {
    TCCR1B |= ( (1 << 1) | (1 << 3) ); 
    TIMSK1 |= (1 << OCIE1A);
  }

  if(!tiltMotorRun)
  {
    TCCR1B &= ~( (1 << 1) | (1 << 3) ); 
    TIMSK1 &= ~(1 << OCIE1A); 
  }
}

//Tilt Interrupts

//Timer Interrupt to set the tilt motor signal at a specified interval 
ISR(TIMER1_COMPA_vect) 
{
  PINA = 0b00000001; // Toggle PA0 (Pin 22) only when set to moving
}

//Timer interrupt to trigger once a falling or rising edge is detected from tilt encoder
ISR(TIMER5_CAPT_vect) 
{
  static uint16_t tilt_lastCapture = 0;
  static bool tilt_measuringOnTime = true;
  uint16_t tilt_currentCapture = ICR5;  // Read Timer5 capture register

  if (tilt_measuringOnTime) 
  {
    tilt_t_on = tilt_currentCapture - tilt_lastCapture;  // Measure high time
    TCCR5B ^= (1 << ICES5);  // Toggle to capture rising edge next
  } 
  else 
  {
    tilt_t_off = tilt_currentCapture - tilt_lastCapture; // Measure low time
    tiltNewData = true; // New data ready
    TCCR5B ^= (1 << ICES5);  // Toggle to capture falling edge next
  }

  tilt_measuringOnTime = !tilt_measuringOnTime; 
  tilt_lastCapture = tilt_currentCapture;
}

void tiltEncoderposition()
{
  if (tiltNewData) 
  {
    cli();  // Disable interrupts 
    uint16_t tilt_highTime = tilt_t_on;  // Copy safely
    uint16_t tilt_lowTime = tilt_t_off;
    tiltNewData = false;
    sei();  // Re-enable interrupts

    float tilt_t_on_us = tilt_highTime * 0.5;
    float tilt_t_off_us = tilt_lowTime * 0.5;

    float x = ((tilt_t_on_us * 1026) / (tilt_t_on_us + tilt_t_off_us)) - 1;

    tiltPosition = (x <= 1022) ? x : 1023;
  }
}

//Pan Interrupts

ISR(TIMER3_COMPA_vect) 
{
  PINA = 0b00000010; // Toggle PA0 (Pin 23) only when set to moving
}

ISR(TIMER4_CAPT_vect) 
{
  static uint16_t pan_lastCapture = 0;
  static bool pan_measuringOnTime = true;
  uint16_t pan_currentCapture = ICR4;  // Read Timer4 capture register

  if (pan_measuringOnTime) 
  {
    pan_t_on = pan_currentCapture - pan_lastCapture;  // Measure high time
    TCCR4B ^= (1 << ICES4);  // Toggle to capture rising edge next
  } 
  else 
  {
    pan_t_off = pan_currentCapture - pan_lastCapture; // Measure low time
    panNewData = true; // New data ready
    TCCR4B ^= (1 << ICES4);  // Toggle to capture falling edge next
  }

  pan_measuringOnTime = !pan_measuringOnTime; 
  pan_lastCapture = pan_currentCapture;
}

void panEncoderPosition()
{
  if (panNewData) 
  {
    cli();  // Disable interrupts 
    uint16_t pan_highTime = pan_t_on;  // Copy safely
    uint16_t pan_lowTime = pan_t_off;
    panNewData = false;
    sei();  // Re-enable interrupts 

    float pan_t_on_us = pan_highTime * 0.5;
    float pan_t_off_us = pan_lowTime * 0.5;

    float x = ((pan_t_on_us * 1026) / (pan_t_on_us + pan_t_off_us)) - 1;

    panPosition = (x <= 1022) ? x : 1023;
  }
}