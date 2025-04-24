// Pan-Tilt Arduino sketch
// – supports absolute (pan,tilt) moves AND continuous CW/CCW & UP/DOWN toggles

#define PAN_STEP_PIN     22
#define PAN_DIR_PIN      24
#define TILT_STEP_PIN    23
#define TILT_DIR_PIN     25
#define STEP_PULSE_DELAY 90  // µs between step pulses

// continuous‐motion state
bool panCont  = false;
int  panDir   = 0;   // +1=CW, -1=CCW

bool tiltCont = false;
int  tiltDir  = 0;   // +1=UP, -1=DOWN

// last absolute targets for vector moves
float lastPan  = 0.0f;  // –180…+180
float lastTilt = 0.0f;  //   0…135

// forward‐declarations
void continuousPanStep();
void continuousTiltStep();
void movePanTilt(float panDelta, float tiltDelta);

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10);
  pinMode(PAN_STEP_PIN,   OUTPUT);
  pinMode(PAN_DIR_PIN,    OUTPUT);
  pinMode(TILT_STEP_PIN,  OUTPUT);
  pinMode(TILT_DIR_PIN,   OUTPUT);
  Serial.println("Ready");
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    // ---- Toggles ----
    if (cmd.equalsIgnoreCase("CW")) {
      panCont = !(panCont && panDir==1);
      panDir  = 1;
      Serial.println(panCont ? "Pan CW started" : "Pan CW stopped");
    }
    else if (cmd.equalsIgnoreCase("CCW")) {
      panCont = !(panCont && panDir==-1);
      panDir  = -1;
      Serial.println(panCont ? "Pan CCW started" : "Pan CCW stopped");
    }
    else if (cmd.equalsIgnoreCase("UP")) {
      tiltCont = !(tiltCont && tiltDir==1);
      tiltDir  = 1;
      Serial.println(tiltCont ? "Tilt UP started" : "Tilt UP stopped");
    }
    else if (cmd.equalsIgnoreCase("DOWN")) {
      tiltCont = !(tiltCont && tiltDir==-1);
      tiltDir  = -1;
      Serial.println(tiltCont ? "Tilt DOWN started" : "Tilt DOWN stopped");
    }

    // ---- Vector move "(pan,tilt)" ----
    else if (cmd.indexOf(',') >= 0) {
      if (cmd.startsWith("(") && cmd.endsWith(")")) {
        cmd = cmd.substring(1, cmd.length()-1);
      }
      int comma = cmd.indexOf(',');
      float newPan  = cmd.substring(0, comma).toFloat();
      float newTilt = cmd.substring(comma+1).toFloat();

      float dPan  = newPan  - lastPan;
      float dTilt = newTilt - lastTilt;
      movePanTilt(dPan, dTilt);

      lastPan  = newPan;
      lastTilt = newTilt;
    }

    // ---- Single‐value fallback = absolute pan only ----
    else {
      float a = cmd.toFloat();
      if (a>=-180 && a<=360) {
        float dPan = a - lastPan;
        movePanTilt(dPan, 0);
        lastPan = a;
      } else {
        Serial.println("Invalid command");
      }
    }
  }

  // continuous stepping
  if (panCont)  continuousPanStep();
  if (tiltCont) continuousTiltStep();
}

void continuousPanStep() {
  digitalWrite(PAN_DIR_PIN, panDir >= 0 ? HIGH : LOW);
  digitalWrite(PAN_STEP_PIN, HIGH);
  delayMicroseconds(STEP_PULSE_DELAY);
  digitalWrite(PAN_STEP_PIN, LOW);
  delayMicroseconds(STEP_PULSE_DELAY);
}

void continuousTiltStep() {
  digitalWrite(TILT_DIR_PIN, tiltDir >= 0 ? HIGH : LOW);
  digitalWrite(TILT_STEP_PIN, HIGH);
  delayMicroseconds(STEP_PULSE_DELAY);
  digitalWrite(TILT_STEP_PIN, LOW);
  delayMicroseconds(STEP_PULSE_DELAY);
}

void movePanTilt(float panDelta, float tiltDelta) {
  // convert to steps
  long panSteps  = lroundf(fabs(panDelta)  / (360.0f / (200*8*13.76f)));
  long tiltSteps = lroundf(fabs(tiltDelta) / (360.0f / (200*8*50.0f)));

  digitalWrite(PAN_DIR_PIN,  panDelta  >= 0 ? HIGH : LOW);
  digitalWrite(TILT_DIR_PIN, tiltDelta >= 0 ? HIGH : LOW);

  long dx = panSteps, dy = tiltSteps;
  long err = dx - dy, e2;
  long cP = 0, cT = 0;

  while (cP < dx || cT < dy) {
    if (Serial.available()) {
      Serial.println("Interrupted");
      return;
    }
    e2 = err*2;
    if (cP<dx && e2>-dy) {
      digitalWrite(PAN_STEP_PIN, HIGH);
      delayMicroseconds(STEP_PULSE_DELAY);
      digitalWrite(PAN_STEP_PIN, LOW);
      delayMicroseconds(STEP_PULSE_DELAY);
      err -= dy;  cP++;
    }
    if (cT<dy && e2<dx) {
      digitalWrite(TILT_STEP_PIN, HIGH);
      delayMicroseconds(STEP_PULSE_DELAY);
      digitalWrite(TILT_STEP_PIN, LOW);
      delayMicroseconds(STEP_PULSE_DELAY);
      err += dx;  cT++;
    }
  }
  Serial.println("Move done");
}
