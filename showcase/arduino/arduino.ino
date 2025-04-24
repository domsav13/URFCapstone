// Pan-Tilt Arduino sketch with absolute slider control
// Sends relative moves based on last commanded position

#define PAN_STEP_PIN        22
#define PAN_DIR_PIN         24
#define TILT_STEP_PIN       23
#define TILT_DIR_PIN        25

#define STEPS_PER_REV       200     // motor full steps per revolution
#define MICROSTEPS          8       // microstepping setting
#define PAN_GEAR_REDUCTION  13.76f  // gearbox ratio
#define TILT_GEAR_REDUCTION 50.0f   // gearbox ratio

#define PAN_DEG_PER_STEP    (360.0f / (STEPS_PER_REV * MICROSTEPS * PAN_GEAR_REDUCTION))
#define TILT_DEG_PER_STEP   (360.0f / (STEPS_PER_REV * MICROSTEPS * TILT_GEAR_REDUCTION))

#define STEP_PULSE_DELAY    90      // microseconds

// continuous‐pan support (if you still use CW/CCW commands)
bool continuousMode       = false;
int  continuousDirection  = 0;    // +1 = CW, -1 = CCW

// track last absolute angles so sliders send deltas
float lastPanAngle        = 0.0f;  // –180…+180
float lastTiltAngle       = 0.0f;  //  0…135

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10);  // don’t block more than 10 ms on Serial.readStringUntil
  pinMode(PAN_STEP_PIN,  OUTPUT);
  pinMode(PAN_DIR_PIN,   OUTPUT);
  pinMode(TILT_STEP_PIN, OUTPUT);
  pinMode(TILT_DIR_PIN,  OUTPUT);
  Serial.println("Ready");
}

/**
 * Move the pan & tilt axes by signed relative angles.
 * panDelta:  relative pan degrees (±360)
 * tiltDelta: relative tilt degrees (±135)
 */
void movePanTilt(float panDelta, float tiltDelta) {
  long dx = lroundf(fabs(panDelta)  / PAN_DEG_PER_STEP);
  long dy = lroundf(fabs(tiltDelta) / TILT_DEG_PER_STEP);

  // direction pins by sign
  digitalWrite(PAN_DIR_PIN,  panDelta  >= 0 ? HIGH : LOW);
  digitalWrite(TILT_DIR_PIN, tiltDelta >= 0 ? HIGH : LOW);

  long err      = dx - dy;
  long curPan   = 0;
  long curTilt  = 0;

  while (curPan < dx || curTilt < dy) {
    // abort if a new command arrives
    if (Serial.available()) {
      Serial.println("Interrupted by new cmd");
      return;
    }

    long e2 = err * 2;
    // step pan
    if (curPan < dx && e2 > -dy) {
      digitalWrite(PAN_STEP_PIN, HIGH);
      delayMicroseconds(STEP_PULSE_DELAY);
      digitalWrite(PAN_STEP_PIN, LOW);
      delayMicroseconds(STEP_PULSE_DELAY);
      err -= dy;
      curPan++;
    }
    // step tilt
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

// optional continuous pan stepping
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

    // legacy continuous commands
    if (cmd.equalsIgnoreCase("CW")) {
      continuousMode      = !(continuousMode && continuousDirection == 1);
      continuousDirection = 1;
      Serial.println(continuousMode
        ? "Continuous CW mode activated"
        : "Continuous CW mode deactivated"
      );
    }
    else if (cmd.equalsIgnoreCase("CCW")) {
      continuousMode      = !(continuousMode && continuousDirection == -1);
      continuousDirection = -1;
      Serial.println(continuousMode
        ? "Continuous CCW mode activated"
        : "Continuous CCW mode deactivated"
      );
    }
    // absolute pan,tilt command "(pan,tilt)"
    else if (cmd.indexOf(',') >= 0) {
      if (cmd.startsWith("(") && cmd.endsWith(")")) {
        cmd = cmd.substring(1, cmd.length() - 1);
      }
      int comma = cmd.indexOf(',');
      float newPan  = cmd.substring(0, comma).toFloat();
      float newTilt = cmd.substring(comma + 1).toFloat();

      // compute signed deltas
      float deltaPan  = newPan  - lastPanAngle;
      float deltaTilt = newTilt - lastTiltAngle;

      // perform the relative move
      movePanTilt(deltaPan, deltaTilt);

      // record new absolute positions
      lastPanAngle  = newPan;
      lastTiltAngle = newTilt;
    }
    // single‐value fallback (absolute pan only)
    else {
      float angle = cmd.toFloat();
      if (angle >= 0 && angle <= 360) {
        float deltaPan = angle - lastPanAngle;
        movePanTilt(deltaPan, 0);
        lastPanAngle = angle;
      } else {
        Serial.println("Invalid command");
      }
    }
  }

  // if in continuous mode, keep stepping
  if (continuousMode) {
    continuousStep();
  }
}
