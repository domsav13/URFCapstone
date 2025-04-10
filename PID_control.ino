volatile uint16_t t_on = 0;  // Stores high-time (PWM on time)
volatile uint16_t t_off = 0; // Stores low-time (PWM off time)
volatile bool newData = false;


volatile uint16_t targetPosition = 400; 
volatile uint16_t position = 0;
volatile bool motorRun = false;

// Store elapsed time (time difference in seconds)
volatile float startTime = 0;  // Timer start value (to calculate elapsed time)
volatile float stopTime = 0;


volatile float error = 0;
volatile float prevError = 0;
volatile float dt = 1;    

volatile float kp = 1;

volatile float ki = 1;
volatile float integral = 1;

volatile float kd = 0.1;
volatile float derivative = 1;

volatile float pid = 1;

volatile uint16_t ocrMax = 65535;  // Maximum value for OCR1A (16-bit timer), (minimum speed)
volatile uint16_t ocrMin = 800; //minimum value for ocr1a (max speed)

volatile float tilt_velocity = 1;

/*
volatile uint8_t send_buf[32];
volatile uint16_t send_len = 0;
volatile bool sending = false;
volatile uint16_t send_index = 0;
*/


void setup() 
{
  cli(); //ensures that interrupts don't interfere this block of code

  //pin 22/23: clock
  //pin 24, 25: direction
  DDRA = 0b00001111;  //sets both pins 22,23,24,25 as output pins
  PORTA = 0b00000000; //sets both pins 22,23,24,25 to start at low

  DDRD  = 0b00000000;
  EICRA = 0b00001111;
  EIMSK = 0b00000011;


  TCCR1A = 0b00000000;  // Normal port operation, CTC mode
  TCCR1B = 0b00001010;  // WGM1[3:2] = 10 (CTC mode), CS1[2:0] = 010 (prescaler 8), sets ctc mode on, resets timer to 0 after reach certain time
  TCCR1C = 0b00000000;

  OCR1A = 999; //compare match value of 100 micro sec (e.g switches between H/L every 100micro or with period 200micro)
  // eqn to calculate above number: (timer clock * signal period / 2 * prescaler) - 1
  //max value i can put here is 65535 (min 15hz freq, max 65ms period), and min is 2 based on the driver limitations (max 500khz freq/min 2 micro period)

  TIMSK1 = 0b00000010; //sets for interrupt to trigger based on the timer 
  
  DDRL &= ~(1 << PL1); //Sets pin 48 as an input

  TCCR5A = 0b00000000;  
  TCCR5B = 0b11000010;  // Input capture on falling edge, Prescaler = 8 (2MHz clock)
  TCCR5C = 0b00000000;

  TIMSK5 = 0b00100001;  // Enable Input Capture and Overflow Interrupts
  
  //timer set up for timeinterval in pid control equation 

  TCCR3A = 0b00000000;  // Normal port operation
  TCCR3B = 0b00000010;  // Prescaler of 8 so timer counts every 8 clock cycles
  TCCR3C = 0b00000000;  // Normal mode

  TCNT3 = 0;  // Set Timer 3 counter to 0
  /*
  // Configure UART Registers
  UCSR0A = 0b00000000;  // Normal speed, no multi-processor mode
  UCSR0B = 0b10011000;  // Enable RX, TX, and RX Complete Interrupt (RXCIE0)
  UCSR0C = 0b00000110;  // 8-bit data, 1 stop bit, no parity

  // Baud Rate Configuration for 9600 baud (for a 16MHz clock)
  UBRR0H = 0b00000000;
  UBRR0L = 103;
  */

  Serial.begin(9600);
  sei();

}

void loop() 
{
  encoderposition();  // Continuously update encoder position
  
  stopTime = TCNT3;
  
  if (TIFR3 & (1 << TOV3)) 
  {
    TIFR3 |= (1 << TOV3);  // Clear the overflow flag by writing 1 to it
    dt = (stopTime + 65536 - startTime) / 2000000;
  }
  else
  {
    dt = (stopTime - startTime) / 2000000;
  }
  
  TCNT3 = 0;

  startTime = TCNT3; 
  
  prevError = error;

  error = (int16_t)position - (int16_t)targetPosition;

  derivative = (error - prevError) / dt ; 

  integral = (prevError * dt) + ( (error - prevError) * dt / 2 );

  pid = (kp * error) + (ki * integral) + (kd * derivative);

  if (pid > 0) //Clockwise
  {
    PORTA |= (1 << 2);
  }
  else //counterclockwise
  {
    PORTA &= ~(1 << 2);
  }

  tilt_velocity = (((ocrMax - ocrMin) / 1024) * abs (pid)) + ocrMin; //motor speed calculation

  if (tilt_velocity <= ocrMax)
  {
    OCR1A = (uint16_t) tilt_velocity ;
  }
  
  Serial.println(position);
  /*
  if (sending) return;  // Prevent starting new transmission if one is ongoing

  send_len = snprintf(send_buf, 32, "%d\n", position);
  send_index = 0;  // Reset index
  sending = true;

  UCSR0B |= (1 << UDRIE0);  // Enable UART Data Register Empty interrupt
  */
}

  

//Timer Interrupt to set the stepper motor signal at a specified interval 
ISR(TIMER1_COMPA_vect) 
{
  PINA = 0b00000011; // Toggle PA0 (Pin 22) and PA1 (Pin 23) only when moving
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
    cli();  // Disable interrupts (equivalent to noInterrupts())
    uint16_t highTime = t_on;  // Copy safely
    uint16_t lowTime = t_off;
    newData = false;
    sei();  // Re-enable interrupts (equivalent to interrupts())

    float t_on_us = highTime * 0.5;
    float t_off_us = lowTime * 0.5;

    float x = ((t_on_us * 1026) / (t_on_us + t_off_us)) - 1;

    position = (x <= 1022) ? x : 1023;
  }
}

/*ISR(USART0_UDRE_vect)
{
  static uint16_t idx = 0;  // Keep track of the current byte being sent

  if (idx < send_len)  // If there are more bytes to send
  {
    UDR0 = send_buf[idx++];  // Load next byte into UART register
  }
  else
  {
    sending = false;  // Mark transmission as complete
    idx = 0;  // Reset index
    UCSR0B &= ~(1 << 5);  // Disable UDRE interrupt to stop sending
  }
}*/
