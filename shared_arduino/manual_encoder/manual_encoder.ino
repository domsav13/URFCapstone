volatile uint16_t t_on = 0;
volatile uint16_t t_off = 0;
volatile bool newData = false;
float targetPosition = -1;
bool motorEnabled = false;
const float POSITION_TOLERANCE = 1.0; // ±1 degree tolerance
unsigned long lastSendTime = 0; // For throttling output

void setup() {
    cli();

    Serial.begin(115200);
    while (!Serial) {
        // ✅ Wait until serial connection is established
    }
    
    // ✅ Confirm that serial is working
    Serial.println("Serial connection established!");
    
    DDRA = 0b00000001; // Pin 22 for step
    PORTA = 0b00000000;

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

        float t_on_us = highTime * 0.5;
        float t_off_us = lowTime * 0.5;

        float x = ((t_on_us * 1026) / (t_on_us + t_off_us)) - 1;
        uint16_t position = (x <= 1022) ? x : 1023;

        // ✅ Convert encoder value to degrees (0 to 360)
        float currentAngle = (position * 360.0) / 1024.0;

        // ✅ Debug output to confirm loop is running
        Serial.print("Angle: ");
        Serial.println(currentAngle);

        // ✅ Throttle output to avoid spam
        if (millis() - lastSendTime > 50) { // Every 50ms
            lastSendTime = millis();
            Serial.println(currentAngle);
        }

        // ✅ Stop motor within tolerance
        if (targetPosition != -1) {
            if (abs(currentAngle - targetPosition) <= POSITION_TOLERANCE) {
                motorEnabled = false;
                PORTA = 0b00000000;
                Serial.println("Target reached");
            } else {
                motorEnabled = true;
            }
        }
    }
}

void loop() {
    // ✅ Read target position from serial
    if (Serial.available()) {
        targetPosition = Serial.parseFloat();
        Serial.print("Target Angle Set: ");
        Serial.println(targetPosition);

        if (targetPosition != -1) {
            motorEnabled = true;
        }
    }

    encoderposition();
}
