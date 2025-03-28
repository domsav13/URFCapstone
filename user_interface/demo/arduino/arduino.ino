#define STEP_PIN 22
#define DIR_PIN 24
#define STEPS_PER_REV 200     // Motor steps per revolution
#define MICROSTEPS 8          // Microstepping setting on motor driver
#define GEAR_REDUCTION 13.76  // Gear reduction factor
#define DEG_PER_STEP (360.0 / (STEPS_PER_REV * MICROSTEPS * GEAR_REDUCTION))
#define STEP_PULSE_DELAY 100  // in microseconds

// Global state for continuous rotation
bool continuousMode = false;
int continuousDirection = 0; // 1 for CW, -1 for CCW

void setup() {
    Serial.begin(115200);
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    Serial.println("Ready");
}

void moveMotor(float degrees) {
    int steps = (int)(degrees / DEG_PER_STEP);
    Serial.print("Moving ");
    Serial.print(steps);
    Serial.println(" steps");

    if (steps >= 0) {
        digitalWrite(DIR_PIN, HIGH); // Clockwise
    } else {
        digitalWrite(DIR_PIN, LOW);  // Counterclockwise
        steps = -steps;
    }

    for (int i = 0; i < steps; i++) {
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(STEP_PULSE_DELAY);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(STEP_PULSE_DELAY);
    }
    Serial.println("Target reached");
}

void continuousStep() {
    // Set direction based on continuousDirection
    if (continuousDirection >= 0) {
        digitalWrite(DIR_PIN, HIGH);
    } else {
        digitalWrite(DIR_PIN, LOW);
    }
    // Pulse one step
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(STEP_PULSE_DELAY);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(STEP_PULSE_DELAY);
}

void loop() {
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();  // Remove extra whitespace

        if (cmd.equalsIgnoreCase("CW")) {
            // If already in continuous CW mode, toggle off.
            if (continuousMode && continuousDirection == 1) {
                continuousMode = false;
                Serial.println("Continuous CW mode deactivated");
                delay(100);  // Pause to let motor settle
            } else {
                // If currently running in CCW, stop first.
                if (continuousMode && continuousDirection != 1) {
                    continuousMode = false;
                    delay(100);  // Pause before switching
                }
                continuousMode = true;
                continuousDirection = 1;
                Serial.println("Continuous CW mode activated");
            }
        } else if (cmd.equalsIgnoreCase("CCW")) {
            // If already in continuous CCW mode, toggle off.
            if (continuousMode && continuousDirection == -1) {
                continuousMode = false;
                Serial.println("Continuous CCW mode deactivated");
                delay(100);
            } else {
                // If currently running in CW, stop first.
                if (continuousMode && continuousDirection != -1) {
                    continuousMode = false;
                    delay(100);
                }
                continuousMode = true;
                continuousDirection = -1;
                Serial.println("Continuous CCW mode activated");
            }
        } else {
            // Otherwise, try to parse as an angle for a fixed move.
            float angle = cmd.toFloat();
            if (angle >= 0 && angle <= 360) {
                moveMotor(angle);
            } else {
                Serial.println("Invalid command");
            }
        }
    }

    // If continuous mode is active, take one continuous step on each loop iteration.
    if (continuousMode) {
        continuousStep();
    }
}
