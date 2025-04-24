#include <Arduino.h>

#define PAN_STEP_PIN   22
#define PAN_DIR_PIN    24
#define TILT_STEP_PIN  23
#define TILT_DIR_PIN   25
#define STEP_DELAY_US  90  // µs between step pulses

bool panCont   = false;
int  panDir    = 1;  // +1 = UP, -1 = DOWN

bool tiltCont  = false;
int  tiltDir   = 1;  // +1 = CW, -1 = CCW

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
  // PAN motor: UP / DOWN
  if (strcmp(cmd, "UP") == 0) {
    if (!panCont) {
      panCont = true;
      panDir = +1;
      digitalWrite(PAN_DIR_PIN, HIGH);
    } else if (panDir == +1) {
      panCont = false;
    } else {
      panDir = +1;
      digitalWrite(PAN_DIR_PIN, HIGH);
    }
  }
  else if (strcmp(cmd, "DOWN") == 0) {
    if (!panCont) {
      panCont = true;
      panDir = -1;
      digitalWrite(PAN_DIR_PIN, LOW);
    } else if (panDir == -1) {
      panCont = false;
    } else {
      panDir = -1;
      digitalWrite(PAN_DIR_PIN, LOW);
    }
  }
  // TILT motor: CW / CCW
  else if (strcmp(cmd, "CW") == 0) {
    if (!tiltCont) {
      tiltCont = true;
      tiltDir = +1;
      digitalWrite(TILT_DIR_PIN, HIGH);
    } else if (tiltDir == +1) {
      tiltCont = false;
    } else {
      tiltDir = +1;
      digitalWrite(TILT_DIR_PIN, HIGH);
    }
  }
  else if (strcmp(cmd, "CCW") == 0) {
    if (!tiltCont) {
      tiltCont = true;
      tiltDir = -1;
      digitalWrite(TILT_DIR_PIN, LOW);
    } else if (tiltDir == -1) {
      tiltCont = false;
    } else {
      tiltDir = -1;
      digitalWrite(TILT_DIR_PIN, LOW);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PAN_STEP_PIN, OUTPUT);
  pinMode(PAN_DIR_PIN,  OUTPUT);
  pinMode(TILT_STEP_PIN, OUTPUT);
  pinMode(TILT_DIR_PIN,  OUTPUT);
}

void loop() {
  // handle incoming serial input
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      buf[idx] = '\0';
      handleCmd(buf);
      idx = 0;
    } else if (c != '\r' && idx < BUF_SZ - 1) {
      buf[idx++] = c;
    }
  }

  // continuous pan stepping
  if (panCont) {
    stepPin(PAN_STEP_PIN);
  }

  // continuous tilt stepping
  if (tiltCont) {
    stepPin(TILT_STEP_PIN);
  }
}
