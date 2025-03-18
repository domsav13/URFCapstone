volatile uint16_t t_on = 0;
volatile uint16_t t_off = 0;
volatile bool newData = false;

int targetPosition = -1; // Target angle (in encoder units)
int motorDirection = 0;  // 1 = rotating, 0 = stopped
bool manualMode = false;

void setup() 
{
    cli(); // Disable interrupts while setting up

    Serial.begin(115200);

    DDRA = 0b00000011; // Set both pins 22 and 23 as output
    PORTA = 0b00000000; // Start with both pins low

    // External interrupt setup
    DDRD  = 0b00000000;
    EICRA = 0b00001111; // Rising and falling edge detection
    EIMSK = 0b00000011;

    // Timer1 setup for motor stepping
    TCCR1A = 0b00000000; // Normal mode
    TCCR1B = 0b00001010; // CTC mode, prescaler = 8
    TCCR1C = 0b00000000;

    OCR1A = 999; // Compare match value = 100 microseconds
    TIMSK1 = 0b00000010; // Enable Timer1 compare interrupt
  
    // Timer5 setup for encoder capture
    DDRL &= ~(1 << PL1); // Pin 48 as input
    TCCR5A = 0b00000000;
    TCCR5B = 0b11000010; // Falling edge, prescaler = 8 (2 MHz)
    TCCR5C = 0b00000000;
    TIMSK5 = 0b00100001; // Enable capture and overflow interrupts

    sei(); // Enable interrupts
}

void loop() 
{
    encoderposition();  // Continuously update encoder position

    // ✅ Read target position from serial
    if (Serial.available()) {
        int newTarget = Serial.parseInt();
        delay(10);
        if (newTarget >= 0 && newTarget <= 360) {
            targetPosition = map(newTarget, 0, 360, 0, 1023);
            manualMode = true;
            Serial.print("New target angle: ");
            Serial.println(newTarget);
        }
    }

    // ✅ Adjust motor to target position
    if (manualMode) {
        adjustMotorPosition();
    }
}

// ✅ Timer Interrupt to step motor signal at fixed interval
ISR(TIMER1_COMPA_vect) 
{
    if (motorDirection == 1) {
        PINA = 0b00000011; // ✅ Toggle pins to create motor step signal
    }
}

// ✅ Timer interrupt to capture encoder signal
ISR(TIMER5_CAPT_vect) 
{
    static uint16_t lastCapture = 0;
    static bool measuringOnTime = true;
    uint16_t currentCapture = ICR5;

    if (measuringOnTime) 
    {
        t_on = currentCapture - lastCapture;
        TCCR5B ^= (1 << ICES5); // Toggle edge detection
    } 
    else 
    {
        t_off = currentCapture - lastCapture;
        newData = true;
        TCCR5B ^= (1 << ICES5);
    }

    measuringOnTime = !measuringOnTime;
    lastCapture = currentCapture;
}

// ✅ Continuously update encoder position
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

        float x = ((t_on_us * 1026) / (t_on_us + t_off_us)) - 1;
        uint16_t position = (x <= 1022) ? x : 1023;

        // ✅ Send clean CSV output (t_on_us, t_off_us, position)
        Serial.print((int)t_on_us);
        Serial.print(",");
        Serial.print((int)t_off_us);
        Serial.print(",");
        Serial.println(position);
    }
}

// ✅ Rotate motor to target position
void adjustMotorPosition()
{
    // ✅ Read current position from encoder
    float t_on_us = t_on * 0.5;
    float t_off_us = t_off * 0.5;
    float x = ((t_on_us * 1026) / (t_on_us + t_off_us)) - 1;
    uint16_t currentPosition = (x <= 1022) ? x : 1023;

    if (currentPosition == targetPosition) {
        // ✅ Stop motor when target is reached
        motorDirection = 0;
        Serial.println("Target position reached.");
        manualMode = false; // ✅ Stop manual mode once target is reached
    } 
    else {
        // ✅ Always rotate in one direction until target reached
        motorDirection = 1;
        Serial.println("Rotating...");
    }
}
