volatile uint16_t t_on = 0;
volatile uint16_t t_off = 0;
volatile bool newData = false;
float targetPosition = -1;
bool motorEnabled = false;
const float POSITION_TOLERANCE = 1.0; 
unsigned long lastSendTime = 0;
bool targetReached = false;

#define STEP_PIN 22
#define ENABLE_PIN 23

void setup() {
    cli();

    Serial.begin(115200);
    
    // ✅ Add a small delay to prevent reset loop
    delay(100);

    // ✅ Wait for serial connection to stabilize
    while (!Serial) {
        delay(10);
    }

    // ✅ Confirm serial is working
    Serial.println("Serial connection established!");

    pinMode(STEP_PIN, OUTPUT);
    pinMode(ENABLE_PIN, OUTPUT);

    // ✅ Enable motor driver (LOW = enabled for many drivers)
    digitalWrite(ENABLE_PIN, LOW);

    EICRA = 0b00001111;
    EIMSK = 0b00000011;

    // ✅ Step signal at ~100 Hz
    TCCR1A = 0b00000000;
    TCCR1B = 0b00001011;  // Prescaler = 64 → slower stepping
    TCCR1C = 0b00000000;
    OCR1A = 2499; // (16MHz / (64 * 100 Hz)) - 1
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
        // ✅ Toggle step pin to create step signal
        digitalWrite(STEP_PIN, !digitalRead(STEP_PIN));
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

        // ✅ Convert to degrees (0 to 360)
        float currentAngle = (position * 360.0) / 1024.0;

        // ✅ Send data every 50ms
        if (millis() - lastSendTime > 50) {
            lastSendTime = millis();

            // ✅ Only send data if motor is moving
            if (!targetReached) {
                Serial.println(currentAngle);
            }
        }

        // ✅ Stop motor when within tolerance
        if (targetPosition != -1) {
            if (abs(currentAngle - targetPosition) <= POSITION_TOLERANCE) {
                if (motorEnabled) {
                    motorEnabled = false;
                    digitalWrite(STEP_PIN, LOW);
                    Serial.println("Target reached");
                }
                targetReached = true;
            } else {
                if (!motorEnabled) {
                    motorEnabled = true;
                    targetReached = false;
                }
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
            targetReached = false;

            // ✅ Enable driver explicitly
            digitalWrite(ENABLE_PIN, LOW);
        }
    }

    encoderposition();
}
