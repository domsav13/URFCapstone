#define PAN_STEP_PIN 22
#define PAN_DIR_PIN 24
#define TILT_STEP_PIN 23
#define TILT_DIR_PIN 25
#define STEPS_PER_REV 200     // Motor steps per revolution
#define MICROSTEPS 8          // Microstepping setting on motor driver
#define PAN_GEAR_REDUCTION 13.76f  // Pan gear reduction factor
#define TILT_GEAR_REDUCTION 50.0f  // Tilt gear reduction factor
#define PAN_DEG_PER_STEP (360.0f / (STEPS_PER_REV * MICROSTEPS * PAN_GEAR_REDUCTION))
#define TILT_DEG_PER_STEP (135.0f / (STEPS_PER_REV * MICROSTEPS * TILT_GEAR_REDUCTION))
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

/**
 * Move pan and tilt simultaneously using a Bresenham-like algorithm.
 * panDeg: relative pan angle in degrees (-360 to 360)
 * tiltDeg: relative tilt angle in degrees (0 to 135)
 */
void movePanTilt(float panDeg, float tiltDeg) {
    // Constrain tilt between 0 and 135 degrees
    if (tiltDeg < 0) tiltDeg = 0;
    if (tiltDeg > 135) tiltDeg = 135;

    // Calculate step counts (rounded)
    long panSteps  = (long)(fabs(panDeg)  / PAN_DEG_PER_STEP + 0.5f);
    long tiltSteps = (long)(tiltDeg / TILT_DEG_PER_STEP + 0.5f);

    // Set directions
    digitalWrite(PAN_DIR_PIN,  panDeg >= 0 ? HIGH : LOW);
    digitalWrite(TILT_DIR_PIN, tiltDeg >= 0 ? HIGH : LOW);

    long dx = panSteps;
    long dy = tiltSteps;
    long err = dx - dy;
    long curPan  = 0;
    long curTilt = 0;

    // Step both axes until each reaches its target
    while (curPan < dx || curTilt < dy) {
        long e2 = err * 2;
        
        // Pan step
        if (curPan < dx && e2 > -dy) {
            digitalWrite(PAN_STEP_PIN, HIGH);
            delayMicroseconds(STEP_PULSE_DELAY);
            digitalWrite(PAN_STEP_PIN, LOW);
            delayMicroseconds(STEP_PULSE_DELAY);
            err -= dy;
            curPan++;
        }
        
        // Tilt step
        if (curTilt < dy && e2 < dx) {
            digitalWrite(TILT_STEP_PIN, HIGH);
            delayMicroseconds(STEP_PULSE_DELAY);
            digitalWrite(TILT_STEP_PIN, LOW);
            delayMicroseconds(STEP_PULSE_DELAY);
            err += dx;
            curTilt++;
        }
    }

    Serial.println("Pan/Tilt target reached");
}

// Legacy continuous pan stepping
void continuousStep() {
    digitalWrite(PAN_DIR_PIN, continuousDirection >= 0 ? HIGH : LOW);
    digitalWrite(PAN_STEP_PIN, HIGH);
    delayMicroseconds(STEP_PULSE_DELAY);
    digitalWrite(PAN_STEP_PIN, LOW);
    delayMicroseconds(STEP_PULSE_DELAY);
}

void loop() {
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        // Continuous pan commands
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
        // Pan/Tilt vector command (pan,tilt)
        else if (cmd.indexOf(',') >= 0) {
            if (cmd.startsWith("(") && cmd.endsWith(")")) {
                cmd = cmd.substring(1, cmd.length() - 1);
            }
            int comma = cmd.indexOf(',');
            float panAngle  = cmd.substring(0, comma).toFloat();
            float tiltAngle = cmd.substring(comma + 1).toFloat();
            movePanTilt(panAngle, tiltAngle);
        }
        // Single pan angle fallback
        else {
            float angle = cmd.toFloat();
            if (angle >= 0 && angle <= 360) {
                movePanTilt(angle, 0);
            } else {
                Serial.println("Invalid command");
            }
        }
    }

    // Perform continuous pan if active
    if (continuousMode) {
        continuousStep();
    }
}
