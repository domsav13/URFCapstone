#define STEP_PIN 22
#define DIR_PIN 24
#define STEPS_PER_REV 200     // Motor steps per revolution
#define MICROSTEPS 8          // Microstepping setting on motor driver
#define GEAR_REDUCTION 13.76  // Gear reduction factor
#define DEG_PER_STEP (360.0 / (STEPS_PER_REV * MICROSTEPS * GEAR_REDUCTION))

// Adjust these delay values to increase speed (ensure they're within safe limits for your driver)
#define STEP_PULSE_DELAY 200  // in microseconds (adjust from 500 to a lower value)

void setup() {
    Serial.begin(115200);
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    Serial.println("Ready");
}

void moveMotor(float degrees) {
    // Calculate steps needed at the output shaft
    int steps = (int)(degrees / DEG_PER_STEP);

    Serial.print("Moving ");
    Serial.print(steps);
    Serial.println(" steps");

    // Set direction: HIGH for one direction, LOW for the other.
    if (steps >= 0) {
        digitalWrite(DIR_PIN, HIGH); // e.g., clockwise
    } else {
        digitalWrite(DIR_PIN, LOW);  // e.g., counterclockwise
        steps = -steps;  // Convert to a positive number for looping.
    }

    // Pulse the STEP_PIN with shorter delay for increased speed.
    for (int i = 0; i < steps; i++) {
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(STEP_PULSE_DELAY);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(STEP_PULSE_DELAY);
    }

    Serial.println("Target reached");
}

void loop() {
    if (Serial.available()) {
        float degrees = Serial.parseFloat();

        // Flush extra characters
        while (Serial.available()) {
            Serial.read();
        }

        if (degrees >= 0 && degrees <= 360) {
            moveMotor(degrees);
            delay(100);  // Short delay to avoid retriggering
        }
    }
}
