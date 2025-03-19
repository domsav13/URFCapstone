volatile uint16_t t_on = 0;
volatile uint16_t t_off = 0;
volatile bool newData = false;
int targetPosition = -1;

void setup() {
    cli();

    Serial.begin(115200);

    DDRA = 0b00000011;  // Set pins 22 and 23 as outputs
    PORTA = 0b00000000; // Start with low signals

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

ISR(TIMER1_COMPA_vect) {
    PINA = 0b00000011;
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

        float t_on_us = highTime * 0.5;
        float t_off_us = lowTime * 0.5;

        float x = ((t_on_us * 1026) / (t_on_us + t_off_us)) - 1;
        uint16_t position = (x <= 1022) ? x : 1023;

        Serial.print((int)t_on_us);
        Serial.print(",");
        Serial.print((int)t_off_us);
        Serial.print(",");
        Serial.println(position);

        // Move toward target position
        if (targetPosition != -1) {
            if (position < targetPosition) {
                PORTA = 0b00000001; // Step in one direction
            } else if (position > targetPosition) {
                PORTA = 0b00000010; // Step in the opposite direction
            } else {
                PORTA = 0b00000000; // Stop when target is reached
            }
        }
    }
}

void loop() {
    // Read target position from serial
    if (Serial.available()) {
        targetPosition = Serial.parseInt();
        Serial.print("Target Position Set: ");
        Serial.println(targetPosition);
    }

    encoderposition();
}
