volatile char rx_buffer[4];  // enough for 0-100 and "f" or "r" or "u" or "d"
volatile uint8_t rx_index = 0;
bool tiltMotorRun = false;

volatile uint16_t tilt_t_on = 0;  // Stores high-time (PWM on time)
volatile uint16_t tilt_t_off = 0; // Stores low-time (PWM off time)
volatile bool tiltNewData = false;

volatile uint16_t tiltPosition = 0;

volatile uint8_t send_buf[8];
volatile uint16_t send_len = 0;
volatile bool sending = false;

void setup() 
{
  DDRA = 0b00001111; //pin 23,25 output
  PORTA = 0b00001010; //pin 23, 25 default high

  //Timer 3 Setup to trigger at specified interval 
  TCCR3A = 0b00000000;  // Normal port operation, CTC mode
  TCCR3B = 0b00001010;  // WGM1[3:2] = 10 (CTC mode), CS1[2:0] = 010 (prescaler 8), sets ctc mode on, resets timer to 0 after reach certain time
  TCCR3C = 0b00000000;

  OCR3A = 225; //225~325 max run smooth

  TIMSK3 = 0b00000010; //sets for interrupt to trigger based on the timer

  //Timer 4 Setup to trigger at specified interval 
  TCCR4A = 0b00000000;  // Normal port operation, CTC mode
  TCCR4B = 0b00001010;  // WGM1[3:2] = 10 (CTC mode), CS1[2:0] = 010 (prescaler 8), sets ctc mode on, resets timer to 0 after reach certain time
  TCCR4C = 0b00000000;

  OCR4A = 499; //225~325 max run smooth

  TIMSK4 = 0b00000010; //sets for interrupt to trigger based on the timer

  //Setup UART
  UCSR0A = 0b10000000;
  UCSR0B = 0b10011000;  // Enable receive interrupt (TXEN0), enable receiver (UDREIE0)
  UCSR0C = 0b00000110;
  
  UBRR0H = 0;
  UBRR0L = 103;//9600 Baud Rate
}

void loop() 
{

}

ISR(TIMER3_COMPA_vect) 
{
  PINA = 0b00000010; // Toggle PA1 (Pin 23) to move
}

ISR(TIMER4_COMPA_vect) 
{
  if(tiltMotorRun)
  {
    PINA = 0b00000001; // Toggle PA0 (Pin 22) to move
  }
}

ISR(USART0_RX_vect)
{
  char received = UDR0;

  if (received == 'w' || received == 'a' || received == 's' || received == 'd' || received == 'o' ) 
  {
    if (received == 'a') 
    {
      PORTA &= ~(1 << 3);  // Clear bit 3
    }
    else if (received == 'd') 
    {
      PORTA |= (1 << 3);   // Set bit 3
    }

    else if (received == 's') 
    {
      PORTA &= ~(1 << 2);  // Clear bit 1
      tiltMotorRun = true;
    }

    else if (received == 'w') 
    {
      PORTA |= (1 << 2);  // Set bit 1
      tiltMotorRun = true;
    }

    else if (received == 'o') 
    {
      tiltMotorRun = false;
    }

  } 
}