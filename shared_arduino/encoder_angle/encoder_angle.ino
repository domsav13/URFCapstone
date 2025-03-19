#define STEP_PIN 22
#define STEPS_PER_REV 200
#define MICROSTEPS 8
#define DEG_PER_STEP (360.0 / (STEPS_PER_REV * MICROSTEPS))

volatile uint16_t t_on = 0;  
volatile uint16_t t_off = 0; 
volatile bool newData = false;

float targetPosition = -1;
const float POSITION_TOLERANCE = 1.0;

void setup() {
    Serial.begin(115200);
    pinMode(STEP_PIN, OUTPUT);

    // Encoder setup
    TCCR5A = 0b00000000;
    TCCR5B = 0b11000010; // Falling edge, prescaler = 8
    TCCR5C = 0b00000000;
    TIMSK5 = 0b00100001; // Enable input capture interrupt + overflow interrupt
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

float readEncoderAngle() {
    if (newData) {
        cli();
        uint16_t highTime = t_on;
        uint16_t lowTime = t_off;
        newData = false;
        sei();

        float t_on_us = highTime * 0.5;
        float t_off_us = lowTime * 0.5;

        // Convert to absolute position (0–1023) → degrees (0–360)
        float x = ((t_on_us * 1026) / (t_on_us + t_off_us)) - 1;
        uint16_t position = (x <= 1022) ? x : 1023;

        float angle = (position * 360.0) / 1024.0;
        return angle;
    }
    return -1;
}

void moveMotor(float target) {
    float currentAngle = readEncoderAngle();

    if (currentAngle < 0) return; // Encoder data not ready

    Serial.print("Current Angle: ");
    Serial.print(currentAngle);
    Serial.print(", Target: ");
    Serial.println(target);

    int steps;
    if (target > currentAngle) {
        steps = (int)((target - currentAngle) / DEG_PER_STEP);
    } else {
        steps = (int)((currentAngle - target) / DEG_PER_STEP);
    }

    if (steps > 0) {
        for (int i = 0; i < steps; i++) {
            digitalWrite(STEP_PIN, HIGH);
            delayMicroseconds(500);
            digitalWrite(STEP_PIN, LOW);
            delayMicroseconds(500);

            currentAngle = readEncoderAngle();

            // ✅ Stop if within tolerance
            if (abs(currentAngle - target) <= POSITION_TOLERANCE) {
                Serial.println("Target reached");
                break;
            }
        }
    }
}

void loop() {
    if (Serial.available()) {
        targetPosition = Serial.parseFloat();

        // ✅ Flush extra characters
        while (Serial.available()) {
            Serial.read();
        }

        if (targetPosition >= 0 && targetPosition <= 360) {
            moveMotor(targetPosition);
            delay(100);
        }
    }
}
