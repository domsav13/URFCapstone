volatile uint16_t t_on = 0;
volatile uint16_t t_off = 0;
volatile bool newData = false;

int targetPosition = -1;  // Target angle (0–360°)
int motorDirection = 0;   // 1 = CW, -1 = CCW, 0 = stopped
bool manualMode = false;  // Enable manual mode when target set

void setup() 
{
    cli(); 

    Serial.begin(115200);

    DDRA = 0b00000011;
    PORTA = 0b00000000;

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

    sei();
}

void loop() 
{
    encoderposition();  

    if (Serial.available()) {
        int newTarget = Serial.parseInt();
        if (newTarget >= 0 && newTarget <= 360) {
            targetPosition = map(newTarget, 0, 360, 0, 1023); // Map 360 deg to encoder range
            manualMode = true;
            Serial.print("New target angle: ");
            Serial.println(newTarget);
        }
    }

    // If in manual mode, adjust motor position
    if (manualMode) {
        adjustMotorPosition();
    }
}

// Timer Interrupt to set stepper motor signal at interval
ISR(TIMER1_COMPA_vect) 
{
    if (motorDirection != 0) {
        PINA = 0b00000011; // Toggle PA0 (Pin 22) and PA1 (Pin 23) between H/L
    }
}

// Timer interrupt to trigger on encoder pulse
ISR(TIMER5_CAPT_vect) 
{
    static uint16_t lastCapture = 0;
    static bool measuringOnTime = true;
    uint16_t currentCapture = ICR5;

    if (measuringOnTime) 
    {
        t_on = currentCapture - lastCapture;
        TCCR5B ^= (1 << ICES5);
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

        Serial.print((int)t_on_us);
        Serial.print(",");
        Serial.print((int)t_off_us);
        Serial.print(",");
        Serial.println(position);
    }
}

void adjustMotorPosition()
{
    // Read current position
    float t_on_us = t_on * 0.5;
    float t_off_us = t_off * 0.5;
    float x = ((t_on_us * 1026) / (t_on_us + t_off_us)) - 1;
    uint16_t currentPosition = (x <= 1022) ? x : 1023;

    if (currentPosition == targetPosition) {
        // Stop motor if at target
        motorDirection = 0;
        Serial.println("Target position reached.");
    } 
    else {
        int diffClockwise = (targetPosition - currentPosition + 1024) % 1024;
        int diffCounterClockwise = (currentPosition - targetPosition + 1024) % 1024;

        if (diffClockwise < diffCounterClockwise) {
            motorDirection = 1; // Rotate CW
            Serial.println("Rotating CW");
        } 
        else {
            motorDirection = -1; // Rotate CCW
            Serial.println("Rotating CCW");
        }
    }
}
