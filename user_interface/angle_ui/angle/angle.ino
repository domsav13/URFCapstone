#define STEP_PIN 22
#define DIR_PIN 24
#define STEPS_PER_REV 200     // Motor steps per revolution
#define MICROSTEPS 8          // Microstepping setting on motor driver
#define GEAR_REDUCTION 13.76  // Gear reduction factor
// Calculate effective degrees per step at the output shaft
#define DEG_PER_STEP (360.0 / (STEPS_PER_REV * MICROSTEPS * GEAR_REDUCTION))

void setup() {
    Serial.begin(115200);
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    Serial.println("Ready");
}

void moveMotor(float degrees) {
    // Calculate the number of steps needed for the output shaft to move by "degrees"
    int steps = (int)(degrees / DEG_PER_STEP);

    Serial.print("Moving ");
    Serial.print(steps);
    Serial.println(" steps");

    // Determine direction: positive steps for one direction, negative for the other.
    if (steps >= 0) {
        digitalWrite(DIR_PIN, HIGH); // Set direction to clockwise (example)
    } else {
        digitalWrite(DIR_PIN, LOW);  // Set direction to counterclockwise (example)
        steps = -steps;  // Convert to a positive number for stepping.
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
        while (Serial.available()) {  // Flush extra characters.
            Serial.read();
        }
        if (degrees >= 0 && degrees <= 360) {
            moveMotor(degrees);
            delay(100);  // Short delay to avoid retriggering
        }
    }
}
