#define STEP_PIN 22
#define DIR_PIN 24
#define STEPS_PER_REV 200     // Adjust based on stepper motor specs
#define MICROSTEPS 8          // Microstepping setting on motor driver
#define DEG_PER_STEP (360.0 / (STEPS_PER_REV * MICROSTEPS))

void setup() {
    Serial.begin(115200);
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    Serial.println("Ready");
}

void moveMotor(float degrees) {
    // Calculate the number of steps needed.
    // Positive steps will mean one direction, negative the other.
    int steps = (int)(degrees / DEG_PER_STEP);

    Serial.print("Moving ");
    Serial.print(steps);
    Serial.println(" steps");

    // Set the direction based on the sign of steps.
    if (steps >= 0) {
        digitalWrite(DIR_PIN, HIGH); // Clockwise, for example.
    } else {
        digitalWrite(DIR_PIN, LOW);  // Counterclockwise.
        steps = -steps;  // Make steps positive for the loop.
    }

    // Pulse the STEP_PIN to move the motor.
    for (int i = 0; i < steps; i++) {
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(500);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(500);
    }

    Serial.println("Target reached");
}

void loop() {
    if (Serial.available()) {
        float degrees = Serial.parseFloat();

        // Flush any remaining characters.
        while (Serial.available()) {
            Serial.read();
        }

        if (degrees >= 0 && degrees <= 360) {
            moveMotor(degrees);
            delay(100);  // Short delay to avoid retriggering
        }
    }
}
