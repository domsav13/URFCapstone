int targetPosition = -1; // Target angle (in encoder units)
bool manualMode = false;

void loop() 
{
    encoderposition();

    // ✅ Read target position from serial
    if (Serial.available()) {
        int newTarget = Serial.parseInt();
        if (newTarget >= 0 && newTarget <= 360) {
            targetPosition = map(newTarget, 0, 360, 0, 1023);
            manualMode = true;
            Serial.print("New target angle: ");
            Serial.println(newTarget);
        }
    }

    // ✅ Adjust motor position in manual mode
    if (manualMode) {
        adjustMotorPosition();
    }
}

void adjustMotorPosition()
{
    float t_on_us = t_on * 0.5;
    float t_off_us = t_off * 0.5;
    float x = ((t_on_us * 1026) / (t_on_us + t_off_us)) - 1;
    uint16_t currentPosition = (x <= 1022) ? x : 1023;

    if (currentPosition == targetPosition) {
        motorDirection = 0;
    } 
    else {
        int diffClockwise = (targetPosition - currentPosition + 1024) % 1024;
        int diffCounterClockwise = (currentPosition - targetPosition + 1024) % 1024;

        if (diffClockwise < diffCounterClockwise) {
            motorDirection = 1;
            Serial.println("Rotating CW");
        } 
        else {
            motorDirection = -1;
            Serial.println("Rotating CCW");
        }
    }
}
