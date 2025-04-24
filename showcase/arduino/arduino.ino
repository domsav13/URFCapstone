#define PAN_STEP_PIN 22
#define PAN_DIR_PIN 24
#define TILT_STEP_PIN 23
#define TILT_DIR_PIN 25
#define STEPS_PER_REV 200     // Motor steps per revolution
#define MICROSTEPS 8          // Microstepping setting on motor driver
#define PAN_GEAR_REDUCTION 13.76f  // Pan gear reduction factor
#define TILT_GEAR_REDUCTION 50.0f  // Tilt gear reduction factor
#define PAN_DEG_PER_STEP (360.0f / (STEPS_PER_REV * MICROSTEPS * PAN_GEAR_REDUCTION))
#define TILT_DEG_PER_STEP (360.0f / (STEPS_PER_REV * MICROSTEPS * TILT_GEAR_REDUCTION))
#define STEP_PULSE_DELAY 90  // in microseconds

// Global state for continuous pan rotation
bool continuousMode = false;
int continuousDirection = 0; // 1 for CW, -1 for CCW

void setup() {
    Serial.begin(115200);
    pinMode(PAN_STEP_PIN, OUTPUT);
    pinMode(PAN_DIR_PIN, OUTPUT);
    pinMode(TILT_STEP_PIN, OUTPUT);
    pinMode(TILT_DIR_PIN, OUTPUT);
    Serial.println("Ready");
}

void movePan(float degrees) {
    int steps = (int)(degrees / PAN_DEG_PER_STEP);
    Serial.print("Moving pan ");
    Serial.print(steps);
    Serial.println(" steps");

    if (steps >= 0) {
        digitalWrite(PAN_DIR_PIN, HIGH);
    } else {
        digitalWrite(PAN_DIR_PIN, LOW);
        steps = -steps;
    }

    for (int i = 0; i < steps; i++) {
        digitalWrite(PAN_STEP_PIN, HIGH);
        delayMicroseconds(STEP_PULSE_DELAY);
        digitalWrite(PAN_STEP_PIN, LOW);
        delayMicroseconds(STEP_PULSE_DELAY);
    }
    Serial.println("Pan target reached");
}

void moveTilt(float degrees) {
    // Constrain tilt between 0 and 135 degrees
    if (degrees < 0) degrees = 0;
    if (degrees > 135) degrees = 135;

    int steps = (int)(degrees / TILT_DEG_PER_STEP);
    Serial.print("Moving tilt ");
    Serial.print(steps);
    Serial.println(" steps");

    if (steps >= 0) {
        digitalWrite(TILT_DIR_PIN, HIGH);
    } else {
        digitalWrite(TILT_DIR_PIN, LOW);
        steps = -steps;
    }

    for (int i = 0; i < steps; i++) {
        digitalWrite(TILT_STEP_PIN, HIGH);
        delayMicroseconds(STEP_PULSE_DELAY);
        digitalWrite(TILT_STEP_PIN, LOW);
        delayMicroseconds(STEP_PULSE_DELAY);
    }
    Serial.println("Tilt target reached");
}

void continuousStep() {
    // Continuous only for pan
    if (continuousDirection >= 0) {
        digitalWrite(PAN_DIR_PIN, HIGH);
    } else {
        digitalWrite(PAN_DIR_PIN, LOW);
    }
    digitalWrite(PAN_STEP_PIN, HIGH);
    delayMicroseconds(STEP_PULSE_DELAY);
    digitalWrite(PAN_STEP_PIN, LOW);
    delayMicroseconds(STEP_PULSE_DELAY);
}

void loop() {
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        // Handle continuous pan commands
        if (cmd.equalsIgnoreCase("CW")) {
            if (continuousMode && continuousDirection == 1) {
                continuousMode = false;
                Serial.println("Continuous CW mode deactivated");
                delay(100);
            } else {
                if (continuousMode && continuousDirection != 1) {
                    continuousMode = false;
                    delay(100);
                }
                continuousMode = true;
                continuousDirection = 1;
                Serial.println("Continuous CW mode activated");
            }
        } else if (cmd.equalsIgnoreCase("CCW")) {
            if (continuousMode && continuousDirection == -1) {
                continuousMode = false;
                Serial.println("Continuous CCW mode deactivated");
                delay(100);
            } else {
                if (continuousMode && continuousDirection != -1) {
                    continuousMode = false;
                    delay(100);
                }
                continuousMode = true;
                continuousDirection = -1;
                Serial.println("Continuous CCW mode activated");
            }
        }
        // Vector command with pan and tilt
        else if (cmd.indexOf(',') >= 0) {
            // Remove parentheses if present
            if (cmd.startsWith("(") && cmd.endsWith(")")) {
                cmd = cmd.substring(1, cmd.length() - 1);
            }
            int comma = cmd.indexOf(',');
            String panStr = cmd.substring(0, comma);
            String tiltStr = cmd.substring(comma + 1);

            float panAngle = panStr.toFloat();
            float tiltAngle = tiltStr.toFloat();

            movePan(panAngle);
            moveTilt(tiltAngle);
        }
        // Single angle for pan fallback
        else {
            float angle = cmd.toFloat();
            if (angle >= 0 && angle <= 360) {
                movePan(angle);
            } else {
                Serial.println("Invalid command");
            }
        }
    }

    if (continuousMode) {
        continuousStep();
    }
}
