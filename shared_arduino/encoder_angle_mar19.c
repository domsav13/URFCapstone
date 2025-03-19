volatile uint16_t t_on = 0;
volatile uint16_t t_off = 0;
volatile bool newData = false;
float targetPosition = -1;
bool motorEnabled = false;
const float POSITION_TOLERANCE = 1.0;  // ±1 degree tolerance

void setup() {
    cli();  // Disable interrupts

    Serial.begin(115200);

    DDRA = 0b00000001;  // Set pin 22 as output for step
    PORTA = 0b00000000; // Start with low signals

    EICRA = 0b00001111;
    EIMSK = 0b00000011;

    TCCR1A = 0b00000000;
    TCCR1B = 0b00001010;
    TCCR1C = 0b00000000;
    OCR1A = 999; // Step signal every 100 microseconds
    TIMSK1 = 0b00000010;

    DDRL &= ~(1 << PL1);

    TCCR5A = 0b00000000;
    TCCR5B = 0b11000010;
    TCCR5C = 0b00000000;
    TIMSK5 = 0b00100001;

    sei();  // Enable interrupts
}

ISR(TIMER1_COMPA_vect) {
    if (motorEnabled) {
        PINA = 0b00000001; // Toggle step pin (digital 22)
    }
}

ISR(TIMER5_CAPT_vect) {
    static uint16_t lastCapture = 0;
    static bool measuringOnTime = true;
    uint16_t currentCapture = ICR5;

    if (measuringOnTime) {
        t_on = currentCapture - lastCapture;
        TCCR5B ^= (1 << ICES5);
    } else {
        t_off = currentCapture - lastCapture;
        newData = true;
        TCCR5B ^= (1 << ICES5);
    }

    measuringOnTime = !measuringOnTime;
    lastCapture = currentCapture;
}

void encoderposition() {
    if (newData) {
        cli();
        uint16_t highTime = t_on;
        uint16_t lowTime = t_off;
        newData = false;
        sei();

        // Convert PWM timing to raw encoder position
        float t_on_us = highTime * 0.5;
        float t_off_us = lowTime * 0.5;

        float x = ((t_on_us * 1026) / (t_on_us + t_off_us)) - 1;
        uint16_t position = (x <= 1022) ? x : 1023;

        // ✅ Convert encoder value to degrees (0 to 360)
        float currentAngle = (position * 360.0) / 1024.0;

        // ✅ Send clean data to C file (angle only)
        Serial.println(currentAngle);

        // ✅ Stop motor when within tolerance range
        if (targetPosition != -1) {
            if (abs(currentAngle - targetPosition) <= POSITION_TOLERANCE) {
                motorEnabled = false;
                PORTA = 0b00000000; // Stop motor
                Serial.println("Target reached"); // Optional for debugging
            } else {
                motorEnabled = true; // Keep stepping if out of tolerance
            }
        }
    }
}

void loop() {
    // ✅ Read target position from serial
    if (Serial.available()) {
        targetPosition = Serial.parseFloat(); // Use float for angle precision
        Serial.print("Target Angle Set: ");
        Serial.println(targetPosition);

        if (targetPosition != -1) {
            motorEnabled = true;
        }
    }

    encoderposition();
}
