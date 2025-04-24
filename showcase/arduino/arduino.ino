// pan_tilt_direction_only.ino
// – only UP/DOWN toggles pan motor (pins 22/24)
// – only CW/CCW toggles tilt motor (pins 23/25)
// – no angle moves, just continuous stepping in selected direction

#include <Arduino.h>

#define PAN_STEP_PIN   22
#define PAN_DIR_PIN    24
#define TILT_STEP_PIN  23
#define TILT_DIR_PIN   25
#define STEP_DELAY_US  90    // adjust up if you still see stutter

bool panCont   = false;
int  panDir    = 1;       // +1 = UP, -1 = DOWN

bool tiltCont  = false;
int  tiltDir   = 1;       // +1 = CW, -1 = CCW

// simple ring buffer for incoming commands
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

void handleCmd(const char* cmd) {
  // PAN control: UP / DOWN
  if (strcmp(cmd, "UP") == 0) {
    if (!panCont) {
      panCont = true;
      panDir  = +1;
    }
    else if (panDir < 0) {
      panDir = +1;
    }
    else {
      panCont = false;
    }
  }
  else if (strcmp(cmd, "DOWN") == 0) {
    if (!panCont) {
      panCont = true;
      panDir  = -1;
    }
    else if (panDir > 0) {
      panDir = -1;
    }
    else {
      panCont = false;
    }
  }

  // TILT control: CW / CCW
  else if (strcmp(cmd, "CW") == 0) {
    if (!tiltCont) {
      tiltCont = true;
      tiltDir  = +1;
    }
    else if (tiltDir < 0) {
      tiltDir = +1;
    }
    else {
      tiltCont = false;
    }
  }
  else if (strcmp(cmd, "CCW") == 0) {
    if (!tiltCont) {
      tiltCont = true;
      tiltDir  = -1;
    }
    else if (tiltDir > 0) {
      tiltDir = -1;
    }
    else {
      tiltCont = false;
    }
  }
  // ignore any other commands
}

void setup() {
  Serial.begin(115200);
  pinMode(PAN_STEP_PIN,   OUTPUT);
  pinMode(PAN_DIR_PIN,    OUTPUT);
  pinMode(TILT_STEP_PIN,  OUTPUT);
  pinMode(TILT_DIR_PIN,   OUTPUT);
}

void loop() {
  // read serial into buffer until newline
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

  // continuous stepping
  if (panCont)   continuousPanStep();
  if (tiltCont)  continuousTiltStep();
}
