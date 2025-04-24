#define PAN_STEP_PIN 22
#define PAN_DIR_PIN  24
#define TILT_STEP_PIN 23
#define TILT_DIR_PIN 25
#define STEPS_PER_REV 200
#define MICROSTEPS    8
#define PAN_GEAR_REDUCTION  13.76f
#define TILT_GEAR_REDUCTION 50.0f
#define PAN_DEG_PER_STEP  (360.0f / (STEPS_PER_REV * MICROSTEPS * PAN_GEAR_REDUCTION))
#define TILT_DEG_PER_STEP (360.0f / (STEPS_PER_REV * MICROSTEPS * TILT_GEAR_REDUCTION))
#define STEP_PULSE_DELAY   90  // µs

bool continuousMode = false;
int  continuousDirection = 0;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10);              // <<— don't block >10 ms waiting for “\n”
  pinMode(PAN_STEP_PIN, OUTPUT);
  pinMode(PAN_DIR_PIN,  OUTPUT);
  pinMode(TILT_STEP_PIN, OUTPUT);
  pinMode(TILT_DIR_PIN,  OUTPUT);
  Serial.println("Ready");
}

void movePanTilt(float panDeg, float tiltDeg) {
  // clamp tilt
  tiltDeg = constrain(tiltDeg, 0.0f, 135.0f);

  long dx = lroundf(fabs(panDeg)  / PAN_DEG_PER_STEP);
  long dy = lroundf(tiltDeg        / TILT_DEG_PER_STEP);
  digitalWrite(PAN_DIR_PIN,  panDeg  >= 0 ? HIGH : LOW);
  digitalWrite(TILT_DIR_PIN, tiltDeg >= 0 ? HIGH : LOW);

  long err      = dx - dy;
  long curPan   = 0;
  long curTilt  = 0;

  while ((curPan < dx || curTilt < dy)) {
    // —— check for an interrupting command ——
    if (Serial.available() > 0) {
      Serial.println("Interrupted by new cmd");
      return;
    }

    long e2 = err * 2;
    // pan step
    if (curPan < dx && e2 > -dy) {
      digitalWrite(PAN_STEP_PIN, HIGH);
      delayMicroseconds(STEP_PULSE_DELAY);
      digitalWrite(PAN_STEP_PIN, LOW);
      delayMicroseconds(STEP_PULSE_DELAY);
      err -= dy;
      curPan++;
    }
    // tilt step
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

    if (cmd.equalsIgnoreCase("CW")) {
      continuousMode = !(continuousMode && continuousDirection == 1);
      continuousDirection = 1;
      Serial.println(continuousMode ? "Continuous CW mode activated"
                                    : "Continuous CW mode deactivated");
    }
    else if (cmd.equalsIgnoreCase("CCW")) {
      continuousMode = !(continuousMode && continuousDirection == -1);
      continuousDirection = -1;
      Serial.println(continuousMode ? "Continuous CCW mode activated"
                                    : "Continuous CCW mode deactivated");
    }
    else if (cmd.indexOf(',') >= 0) {
      if (cmd.startsWith("(") && cmd.endsWith(")"))
        cmd = cmd.substring(1, cmd.length() - 1);
      int comma = cmd.indexOf(',');
      float panAngle  = cmd.substring(0, comma).toFloat();
      float tiltAngle = cmd.substring(comma + 1).toFloat();
      movePanTilt(panAngle, tiltAngle);
    }
    else {
      float angle = cmd.toFloat();
      if (angle >= 0 && angle <= 360)
        movePanTilt(angle, 0);
      else
        Serial.println("Invalid command");
    }
  }

  if (continuousMode) {
    continuousStep();
  }
}
