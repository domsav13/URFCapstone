volatile uint8_t tilt_angle = '45'; //for tilt this will vary between 0 - 135 (45 - Horizon angle)
volatile uint16_t pan_angle = '0'; //for pan this varies from 0-360

volatile char rx_buffer[8];  // enough for "180,360\0"
volatile uint8_t rx_index = 0;

volatile char tx_buffer[11];
volatile uint8_t tx_index = 0; 
volatile bool send_ready = false;

// Prepare tx_buffer with current pan and tilt
static char pan_str[6];
static char tilt_str[6];



void setup() 
{
  cli();
  
  //UART Setup
  UCSR0A = 0b10000000;
  UCSR0B = 0b10011000;  // Enable receive interrupt (TXEN0), enable receiver (UDREIE0)
  UCSR0C = 0b00000110;
  
  //9600 Baud Rate
  UBRR0H = 0;
  UBRR0L = 103;

  //Timer 0 Setup to send data to Serial Monitor 
  TCCR0A = 0b00000010; // CTC mode
  TCCR0B = 0b00000101; // Prescaler = 1024
  OCR0A  = 255;        // Max count value for lowest freq

  TIMSK0 = 0b00000010; // Enable Compare Match A interrupt
  sei();
}

void loop() 
{

}

ISR(USART0_RX_vect) 
{
  char received = UDR0;

  if (rx_index < sizeof(rx_buffer) - 1) 
  {
    if (received == '\n' || received == '\r') 
    {
      rx_buffer[rx_index] = '\0'; // Null terminate

      // Parse using sscanf to split at the comma
      uint16_t a, b;
      if (sscanf((char*)rx_buffer, "%hu,%hu", &a, &b) == 2) 
      {
        if (a <= 360 && b <= 145) 
        {
          pan_angle = a;
          tilt_angle = b;
        }
      }

      rx_index = 0; // Reset buffer
    } 
    else 
    {
      rx_buffer[rx_index++] = received;
    }
  } 
  else 
  {
    // If buffer overflows, reset it
    rx_index = 0;
  }
}

ISR(USART0_UDRE_vect) 
{
  if (send_ready) 
  {
    if (tx_buffer[tx_index] != '\0') 
    {
      UDR0 = tx_buffer[tx_index++];
    } 
    else 
    {
      send_ready = false;
      UCSR0B &= ~(1 << UDRIE0); // Disable interrupt until next send
    }
  }
}

ISR(TIMER0_COMPA_vect) 
{
  if (!send_ready) 
  {
    itoa(pan_angle, pan_str, 10);
    itoa(tilt_angle, tilt_str, 10);

    uint8_t i = 0;
    for (uint8_t j = 0; pan_str[j] != '\0'; ++j) 
    {
      tx_buffer[i++] = pan_str[j];
    }

    tx_buffer[i++] = ',';

    for (uint8_t j = 0; tilt_str[j] != '\0'; ++j) 
    {
      tx_buffer[i++] = tilt_str[j];
    }
    
    tx_buffer[i++] = '\r';
    tx_buffer[i++] = '\n';
    tx_buffer[i] = '\0';
    

    tx_index = 0;
    send_ready = true;
    
    UCSR0B |= (1 << UDRIE0); // Enable TX interrupt
  }
}