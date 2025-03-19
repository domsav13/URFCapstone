volatile uint16_t t_on = 0;  
volatile uint16_t t_off = 0; 
volatile bool newData = false;

unsigned long lastSendTime = 0;  // Track last time angle was sent

void setup() 
{
  cli(); // Disable interrupts temporarily

  Serial.begin(115200);

  DDRA = 0b00000011;  // Set pins 22 and 23 as outputs
  PORTA = 0b00000000; // Set pins 22 and 23 LOW at start

  DDRD  = 0b00000000;
  EICRA = 0b00001111;
  EIMSK = 0b00000011;

  TCCR1A = 0b00000000;
  TCCR1B = 0b00001010;
  TCCR1C = 0b00000000;

  OCR1A = 999;
  TIMSK1 = 0b00000010;
  
  DDRL &= ~(1 << PL1);

  TCCR5A = 0b00000000;
  TCCR5B = 0b11000010;
  TCCR5C = 0b00000000;
  TIMSK5 = 0b00100001;

  sei(); // Enable interrupts
}

void loop() 
{
  encoderposition();  // Continuously update encoder position

  // ✅ Send encoder angle every 1 second
  if (millis() - lastSendTime >= 1000) {
    lastSendTime = millis();

    float angle = readEncoderAngle();
    if (angle >= 0) {
      Serial.print("Angle: ");
      Serial.println(angle);
    }
  }
}

ISR(TIMER1_COMPA_vect) 
{
  PINA = 0b00000011; // Toggle PA0 (Pin 22) and PA1 (Pin 23) between H/L
}

ISR(TIMER5_CAPT_vect) 
{
  static uint16_t lastCapture = 0;
  static bool measuringOnTime = true;
  uint16_t currentCapture = ICR5;

  if (measuringOnTime) {
    t_on = currentCapture - lastCapture;
    TCCR5B ^= (1 << ICES5);
  } 
  else {
    t_off = currentCapture - lastCapture;
    newData = true;
    TCCR5B ^= (1 << ICES5);
  }

  measuringOnTime = !measuringOnTime;
  lastCapture = currentCapture;
}

float readEncoderAngle() {
  if (newData) {
    cli();  
    uint16_t highTime = t_on;
    uint16_t lowTime = t_off;
    newData = false;
    sei();

    float t_on_us = highTime * 0.5;
    float t_off_us = lowTime * 0.5;

    float x = ((t_on_us * 1026) / (t_on_us + t_off_us)) - 1;
    uint16_t position = (x <= 1022) ? x : 1023;

    // ✅ Convert encoder position (0–1023) to angle (0–360 degrees)
    float angle = (position * 360.0) / 1024.0;
    return angle;
  }
  return -1;
}
