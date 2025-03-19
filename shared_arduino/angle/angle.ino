#define STEP_PIN 22
#define STEPS_PER_REV 200 // Adjust based on stepper motor specs
#define MICROSTEPS 8 // Microstepping setting on motor driver
#define DEG_PER_STEP (360.0 / (STEPS_PER_REV * MICROSTEPS))

float targetPosition = -1;
float currentPosition = 0;

void setup() {
    Serial.begin(115200);
    pinMode(STEP_PIN, OUTPUT);

    Serial.println("Ready");
}

void moveMotor(float target) {
    int steps = (int)((target - currentPosition) / DEG_PER_STEP);

    Serial.print("Moving ");
    Serial.print(steps);
    Serial.println(" steps");

    for (int i = 0; i < abs(steps); i++) {
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(500); // Adjust for speed
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(500);

        // Increment or decrement current position
        if (steps > 0) {
            currentPosition += DEG_PER_STEP;
        } else {
            currentPosition -= DEG_PER_STEP;
        }
    }

    Serial.println("Target reached");

    // ✅ Clear target to prevent repeat moves
    targetPosition = -1;
}

void loop() {
    if (Serial.available()) {
        targetPosition = Serial.parseFloat();

        if (targetPosition >= 0 && targetPosition <= 360) {
            moveMotor(targetPosition);
        }
    }
}
