// pan_tilt_interval_noprint.ino
// – one‐way command only, no Serial output

#define PAN_STEP_PIN     22
#define PAN_DIR_PIN      24
#define TILT_STEP_PIN    23
#define TILT_DIR_PIN     25
#define STEP_DELAY_US    90    // µs between step pulses

bool panCont  = false;
int  panDir   = 1;     // +1=CW, -1=CCW

bool tiltCont = false;
int  tiltDir  = 1;     // +1=UP, -1=DOWN

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10);
  pinMode(PAN_STEP_PIN,   OUTPUT);
  pinMode(PAN_DIR_PIN,    OUTPUT);
  pinMode(TILT_STEP_PIN,  OUTPUT);
  pinMode(TILT_DIR_PIN,   OUTPUT);
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    // PAN toggles
    if (cmd.equalsIgnoreCase("CW")) {
      panCont = !panCont;
      panDir  = +1;
    }
    else if (cmd.equalsIgnoreCase("CCW")) {
      panCont = !panCont;
      panDir  = -1;
    }

    // TILT toggles
    else if (cmd.equalsIgnoreCase("UP")) {
      tiltCont = !tiltCont;
      tiltDir  = +1;
    }
    else if (cmd.equalsIgnoreCase("DOWN")) {
      tiltCont = !tiltCont;
      tiltDir  = -1;
    }

    // Vector interval "(dPan,dTilt)"
    else if (cmd.indexOf(',') >= 0) {
      if (cmd.startsWith("(") && cmd.endsWith(")"))
        cmd = cmd.substring(1, cmd.length() - 1);

      int comma = cmd.indexOf(',');
      float dPan  = cmd.substring(0, comma).toFloat();
      float dTilt = cmd.substring(comma + 1).toFloat();
      movePanTilt(dPan, dTilt);
    }

    // Single‐value = pan‐interval only
    else {
      float d = cmd.toFloat();
      movePanTilt(d, 0.0f);
    }
  }

  if (panCont)  continuousPanStep();
  if (tiltCont) continuousTiltStep();
}

void continuousPanStep() {
  digitalWrite(PAN_DIR_PIN, panDir > 0 ? HIGH : LOW);
  digitalWrite(PAN_STEP_PIN, HIGH);
  delayMicroseconds(STEP_DELAY_US);
  digitalWrite(PAN_STEP_PIN, LOW);
  delayMicroseconds(STEP_DELAY_US);
}

void continuousTiltStep() {
  digitalWrite(TILT_DIR_PIN, tiltDir > 0 ? HIGH : LOW);
  digitalWrite(TILT_STEP_PIN, HIGH);
  delayMicroseconds(STEP_DELAY_US);
  digitalWrite(TILT_STEP_PIN, LOW);
  delayMicroseconds(STEP_DELAY_US);
}

void movePanTilt(float dPan, float dTilt) {
  const float panDegPerStep  = 360.0f / (200.0f * 8.0f * 13.76f);
  const float tiltDegPerStep = 360.0f / (200.0f * 8.0f * 50.0f);

  long stepsPan  = lroundf(fabs(dPan)  / panDegPerStep);
  long stepsTilt = lroundf(fabs(dTilt) / tiltDegPerStep);

  digitalWrite(PAN_DIR_PIN,  dPan  >= 0 ? HIGH : LOW);
  digitalWrite(TILT_DIR_PIN, dTilt >= 0 ? HIGH : LOW);

  long dx = stepsPan, dy = stepsTilt;
  long err = dx - dy;

  while (dx > 0 || dy > 0) {
    if (Serial.available()) return;  // immediate abort

    long e2 = err * 2;
    if (dx > 0 && e2 > -dy) {
      digitalWrite(PAN_STEP_PIN, HIGH);
      delayMicroseconds(STEP_DELAY_US);
      digitalWrite(PAN_STEP_PIN, LOW);
      delayMicroseconds(STEP_DELAY_US);
      err -= dy;
      dx--;
    }
    if (dy > 0 && e2 < dx) {
      digitalWrite(TILT_STEP_PIN, HIGH);
      delayMicroseconds(STEP_DELAY_US);
      digitalWrite(TILT_STEP_PIN, LOW);
      delayMicroseconds(STEP_DELAY_US);
      err += dx;
      dy--;
    }
  }
}
