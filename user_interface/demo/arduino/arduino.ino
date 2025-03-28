#define STEP_PIN 22
#define DIR_PIN 24
#define STEPS_PER_REV 200     // Motor steps per revolution
#define MICROSTEPS 8          // Microstepping setting on motor driver
#define GEAR_REDUCTION 13.76  // Gear reduction factor
#define DEG_PER_STEP (360.0 / (STEPS_PER_REV * MICROSTEPS * GEAR_REDUCTION))
#define STEP_PULSE_DELAY 100  // in microseconds

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
        steps = -steps;  // Make steps positive for looping.
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
    // Set the direction for continuous rotation
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
        cmd.trim(); // Remove extra whitespace

        if (cmd.equalsIgnoreCase("CW")) {
            continuousMode = true;
            continuousDirection = 1;
            Serial.println("Continuous CW mode activated");
        } else if (cmd.equalsIgnoreCase("CCW")) {
            continuousMode = true;
            continuousDirection = -1;
            Serial.println("Continuous CCW mode activated");
        } else if (cmd.equalsIgnoreCase("STOP")) {
            continuousMode = false;
            Serial.println("Continuous mode deactivated");
        } else {
            // Try to parse a numeric angle
            float angle = cmd.toFloat();
            if (angle >= 0 && angle <= 360) {
                moveMotor(angle);
            } else {
                Serial.println("Invalid command");
            }
        }
    }
    
    // If in continuous mode, perform one step on each loop iteration.
    if (continuousMode) {
        continuousStep();
    }
}
