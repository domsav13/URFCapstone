// pan_tilt_interval_smooth_improved.ino
// – one-way commands only, no serial output
// – relative intervals: "30", "-15", "(20,-10)"
// – UP/DOWN toggle pan motor (pins 22/24), CW/CCW toggle tilt motor (pins 23/25)
// – pressing opposite direction while running just reverses

#include <Arduino.h>

#define PAN_STEP_PIN   22
#define PAN_DIR_PIN    24
#define TILT_STEP_PIN  23
#define TILT_DIR_PIN   25
#define STEP_DELAY_US  90    // µs between step pulses

bool panCont   = false;
int  panDir    = 1;       // +1=UP, -1=DOWN

bool tiltCont  = false;
int  tiltDir   = 1;       // +1=CW, -1=CCW

constexpr size_t BUF_SZ = 32;
char buf[BUF_SZ];
size_t idx = 0;

inline void stepPin(uint8_t pin) {
  digitalWrite(pin, HIGH);
  delayMicroseconds(STEP_DELAY_US);
  digitalWrite(pin, LOW);
  delayMicroseconds(STEP_DELAY_US);
}

void continuousPanStep() {
  digitalWrite(PAN_DIR_PIN, panDir > 0 ? HIGH : LOW);
  stepPin(PAN_STEP_PIN);
}

void continuousTiltStep() {
  digitalWrite(TILT_DIR_PIN, tiltDir > 0 ? HIGH : LOW);
  stepPin(TILT_STEP_PIN);
}

void movePanTilt(float dPan, float dTilt) {
  const float panStepDeg  = 360.0f / (200.0f * 8.0f * 13.76f);
  const float tiltStepDeg = 360.0f / (200.0f * 8.0f * 50.0f);

  long sPan  = lroundf(fabs(dPan)  / panStepDeg);
  long sTilt = lroundf(fabs(dTilt) / tiltStepDeg);

  digitalWrite(PAN_DIR_PIN,  dPan  >= 0 ? HIGH : LOW);
  digitalWrite(TILT_DIR_PIN, dTilt >= 0 ? HIGH : LOW);

  long dx = sPan, dy = sTilt, err = dx - dy;
  while (dx > 0 || dy > 0) {
    long e2 = err * 2;
    if (dx > 0 && e2 > -dy) {
      stepPin(PAN_STEP_PIN);
      err -= dy; dx--;
    }
    if (dy > 0 && e2 < dx) {
      stepPin(TILT_STEP_PIN);
      err += dx; dy--;
    }
  }
}

void handleCmd(const char* cmd) {
  // PAN: UP/DOWN
  if (strcmp(cmd, "UP") == 0) {
    if (!panCont) {
      panCont = true; panDir = +1;
    } else if (panDir != +1) {
      panDir = +1;
    } else {
      panCont = false;
    }
  }
  else if (strcmp(cmd, "DOWN") == 0) {
    if (!panCont) {
      panCont = true; panDir = -1;
    } else if (panDir != -1) {
      panDir = -1;
    } else {
      panCont = false;
    }
  }
  // TILT: CW/CCW
  else if (strcmp(cmd, "CW") == 0) {
    if (!tiltCont) {
      tiltCont = true; tiltDir = +1;
    } else if (tiltDir != +1) {
      tiltDir = +1;
    } else {
      tiltCont = false;
    }
  }
  else if (strcmp(cmd, "CCW") == 0) {
    if (!tiltCont) {
      tiltCont = true; tiltDir = -1;
    } else if (tiltDir != -1) {
      tiltDir = -1;
    } else {
      tiltCont = false;
    }
  }
  // Relative vector "(dPan,dTilt)"
  else if (strchr(cmd, ',')) {
    char tmp[BUF_SZ];
    strncpy(tmp, cmd, BUF_SZ);
    char* sep = strchr(tmp, ',');
    *sep = '\0';
    float dPan  = atof(tmp);
    float dTilt = atof(sep + 1);
    movePanTilt(dPan, dTilt);
  }
  // Single pan interval
  else {
    float d = atof(cmd);
    movePanTilt(d, 0.0f);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PAN_STEP_PIN,   OUTPUT);
  pinMode(PAN_DIR_PIN,    OUTPUT);
  pinMode(TILT_STEP_PIN,  OUTPUT);
  pinMode(TILT_DIR_PIN,   OUTPUT);
}

void loop() {
  // Read incoming into buffer
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      buf[idx] = '\0';
      handleCmd(buf);
      idx = 0;
    }
    else if (c != '\r' && idx < BUF_SZ - 1) {
      buf[idx++] = c;
    }
  }

  // Continuous stepping
  if (panCont)   continuousPanStep();
  if (tiltCont)  continuousTiltStep();
}
