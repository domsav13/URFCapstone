#define STEP_PIN 22
#define STEPS_PER_REV 200 // Adjust based on stepper motor specs
#define MICROSTEPS 8 // Microstepping setting on motor driver
#define DEG_PER_STEP (360.0 / (STEPS_PER_REV * MICROSTEPS))

float targetPosition = -1;

void setup() {
    Serial.begin(115200);
    pinMode(STEP_PIN, OUTPUT);

    Serial.println("Ready");
}

void moveMotor(float degrees) {
    int steps = (int)(degrees / DEG_PER_STEP);

    Serial.print("Moving ");
    Serial.print(steps);
    Serial.println(" steps");

    for (int i = 0; i < abs(steps); i++) {
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

        // ✅ MANUALLY FLUSH EXTRA CHARACTERS
        while (Serial.available()) {
            Serial.read();
        }

        if (degrees >= 0 && degrees <= 360) {
            moveMotor(degrees);

            // ✅ Short delay to avoid retriggering
            delay(100);
        }
    }
}
