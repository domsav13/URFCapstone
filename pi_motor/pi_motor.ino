#define STEP_PIN 22
bool motorEnabled = false;

void setup() {
    Serial.begin(115200);
    pinMode(STEP_PIN, OUTPUT);
    digitalWrite(STEP_PIN, LOW);
}

void loop() {
    // ✅ Read serial command
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();

        if (command == "ON") {
            motorEnabled = true;
            Serial.println("Motor ON");
        } 
        else if (command == "OFF") {
            motorEnabled = false;
            digitalWrite(STEP_PIN, LOW);  // Ensure motor stops
            Serial.println("Motor OFF");
        }
    }

    // ✅ Generate step signal if motor is enabled
    if (motorEnabled) {
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(1000);  // Adjust for speed
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(1000);
    }
}
