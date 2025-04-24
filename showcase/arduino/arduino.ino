// pan_tilt_direction_toggle.ino
// – UP/DOWN toggles pan motor (pins 22/24)
// – CW/CCW toggles tilt motor (pins 23/25)
// – opposite command reverses direction, same command stops motion
// – no angle moves, no debug prints for smooth stepping

#include <Arduino.h>

#define PAN_STEP_PIN   22
#define PAN_DIR_PIN    24
#define TILT_STEP_PIN  23
#define TILT_DIR_PIN   25
#define STEP_DELAY_US  90    // µs between step pulses

bool panCont   = false;
int  panDir    = 1;       // +1 = UP, -1 = DOWN

bool tiltCont  = false;
int  tiltDir   = 1;       // +1 = CW, -1 = CCW

// command buffer
constexpr size_t BUF_SZ = 32;
char buf[BUF_SZ];
size_t idx = 0;

inline void stepPin(uint8_t pin) {
  digitalWrite(pin, HIGH);
  delayMicroseconds(STEP_DELAY_US);
  digitalWrite(pin, LOW);
  delayMicroseconds(STEP_DELAY_US);
}

void handleCmd(const char* cmd) {
  // PAN: UP / DOWN
  if (strcmp(cmd, "UP") == 0) {
    if (!panCont) {
      panCont = true;
      panDir  = +1;
    } else if (panDir < 0) {
      panDir = +1;
    } else {
      panCont = false;
    }
  }
  else if (strcmp(cmd, "DOWN") == 0) {
    if (!panCont) {
      panCont = true;
      panDir  = -1;
    } else if (panDir > 0) {
      panDir = -1;
    } else {
      panCont = false;
    }
  }
  // TILT: CW / CCW
  else if (strcmp(cmd, "CW") == 0) {
    if (!tiltCont) {
      tiltCont = true;
      tiltDir  = +1;
    } else if (tiltDir < 0) {
      tiltDir = +1;
    } else {
      tiltCont = false;
    }
  }
  else if (strcmp(cmd, "CCW") == 0) {
    if (!tiltCont) {
      tiltCont = true;
      tiltDir  = -1;
    } else if (tiltDir > 0) {
      tiltDir = -1;
    } else {
      tiltCont = false;
    }
  }
  // ignore any other input
}

void setup() {
  Serial.begin(115200);
  pinMode(PAN_STEP_PIN,   OUTPUT);
  pinMode(PAN_DIR_PIN,    OUTPUT);
  pinMode(TILT_STEP_PIN,  OUTPUT);
  pinMode(TILT_DIR_PIN,   OUTPUT);
}

void loop() {
  // read serial until newline
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
  if (panCont) {
    digitalWrite(PAN_DIR_PIN, panDir > 0 ? HIGH : LOW);
    stepPin(PAN_STEP_PIN);
  }
  if (tiltCont) {
    digitalWrite(TILT_DIR_PIN, tiltDir > 0 ? HIGH : LOW);
    stepPin(TILT_STEP_PIN);
  }
}
