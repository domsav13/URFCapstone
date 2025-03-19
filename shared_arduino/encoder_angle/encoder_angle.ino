#define STEP_PIN 22
#define STEPS_PER_REV 200
#define MICROSTEPS 8
#define DEG_PER_STEP (360.0 / (STEPS_PER_REV * MICROSTEPS))

volatile uint16_t t_on = 0;
volatile uint16_t t_off = 0;
volatile bool newData = false;

float targetPosition = -1;
const float POSITION_TOLERANCE = 1.0;
unsigned long lastSendTime = 0;
bool motorEnabled = false;

void setup() {
    Serial.begin(115200);
    pinMode(STEP_PIN, OUTPUT);

    Serial.println("Ready");

    // ✅ Encoder setup
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

    if (currentAngle < 0 || !motorEnabled) return; // Encoder data not ready or motor off

    int steps = (int)((target - currentAngle) / DEG_PER_STEP);

    if (abs(currentAngle - target) > POSITION_TOLERANCE) {
        for (int i = 0; i < abs(steps); i++) {
            // ✅ Stop immediately if motor is turned off
            if (!motorEnabled) return;

            digitalWrite(STEP_PIN, HIGH);
            delayMicroseconds(1000);
            digitalWrite(STEP_PIN, LOW);
            delayMicroseconds(1000);

            currentAngle = readEncoderAngle();

            if (abs(currentAngle - target) <= POSITION_TOLERANCE) {
                Serial.println("Target reached");
                break;
            }
        }
    }
}

void loop() {
    // ✅ Send encoder angle to Pi every second
    if (millis() - lastSendTime >= 1000) {
        lastSendTime = millis();
        float angle = readEncoderAngle();
        if (angle >= 0) {
            Serial.print("Angle: ");
            Serial.println(angle);
        }
    }

    // ✅ Read serial command from Pi
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');

        if (command == "ON") {
            motorEnabled = true;
            Serial.println("Motor enabled");
        } else if (command == "OFF") {
            motorEnabled = false;
            Serial.println("Motor stopped");
        } else {
            targetPosition = command.toFloat();

            if (targetPosition >= 0 && targetPosition <= 360 && motorEnabled) {
                moveMotor(targetPosition);
            }
        }
    }
}
