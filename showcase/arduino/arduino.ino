// Pan-Tilt Arduino sketch with UP/DOWN and CW/CCW toggles

#define PAN_STEP_PIN    22
#define PAN_DIR_PIN     24
#define TILT_STEP_PIN   23
#define TILT_DIR_PIN    25

#define STEP_PULSE_DELAY 90  // µs between step pulses

bool panCont   = false;
int  panDir    = 0;    // +1 = CW, -1 = CCW

bool tiltCont  = false;
int  tiltDir   = 0;    // +1 = UP, -1 = DOWN

void setup() {
  Serial.begin(115200);
  pinMode(PAN_STEP_PIN,   OUTPUT);
  pinMode(PAN_DIR_PIN,    OUTPUT);
  pinMode(TILT_STEP_PIN,  OUTPUT);
  pinMode(TILT_DIR_PIN,   OUTPUT);
  Serial.println("Ready");
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

void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.equalsIgnoreCase("CW")) {
      if (panCont && panDir == 1) {
        panCont = false;
        Serial.println("Pan CW stopped");
      } else {
        panCont = true;
        panDir  = 1;
        Serial.println("Pan CW started");
      }
    }
    else if (cmd.equalsIgnoreCase("CCW")) {
      if (panCont && panDir == -1) {
        panCont = false;
        Serial.println("Pan CCW stopped");
      } else {
        panCont = true;
        panDir  = -1;
        Serial.println("Pan CCW started");
      }
    }
    else if (cmd.equalsIgnoreCase("UP")) {
      if (tiltCont && tiltDir == 1) {
        tiltCont = false;
        Serial.println("Tilt UP stopped");
      } else {
        tiltCont = true;
        tiltDir  = 1;
        Serial.println("Tilt UP started");
      }
    }
    else if (cmd.equalsIgnoreCase("DOWN")) {
      if (tiltCont && tiltDir == -1) {
        tiltCont = false;
        Serial.println("Tilt DOWN stopped");
      } else {
        tiltCont = true;
        tiltDir  = -1;
        Serial.println("Tilt DOWN started");
      }
    }
    else {
      Serial.println("Unknown command");
    }
  }

  // step continuously if enabled
  if (panCont)  continuousPanStep();
  if (tiltCont) continuousTiltStep();
}
