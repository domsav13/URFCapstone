#include <Arduino.h>

#define PAN_STEP_PIN   23
#define PAN_DIR_PIN    25
#define TILT_STEP_PIN  22
#define TILT_DIR_PIN   24
#define STEP_DELAY_US  90    // µs between step pulses

bool panCont   = false;
int  panDir    = 1;       // +1 = CCW, -1 = CW

bool tiltCont  = false;
int  tiltDir   = 1;       // +1 = UP, -1 = DOWN

// buffer for incoming commands
static const size_t BUF_SZ = 32;
char buf[BUF_SZ];
size_t idx = 0;

// one step pulse on given pin
inline void stepPin(uint8_t pin) {
  digitalWrite(pin, HIGH);
  delayMicroseconds(STEP_DELAY_US);
  digitalWrite(pin, LOW);
  delayMicroseconds(STEP_DELAY_US);
}

// handle toggles and angle‐moves
void handleCmd(const char* c) {
  // PAN toggles: CW_ON/CW_OFF, CCW_ON/CCW_OFF
  if (strcmp(c,"CW_ON")==0) {
    panCont = true;
    panDir  = -1;
  } else if (strcmp(c,"CW_OFF")==0) {
    if (panCont && panDir==-1) panCont = false;
  } else if (strcmp(c,"CCW_ON")==0) {
    panCont = true;
    panDir  = +1;
  } else if (strcmp(c,"CCW_OFF")==0) {
    if (panCont && panDir==+1) panCont = false;
  }

  // TILT toggles: UP_ON/UP_OFF, DOWN_ON/DOWN_OFF
  else if (strcmp(c,"UP_ON")==0) {
    tiltCont = true;
    tiltDir  = +1;
  } else if (strcmp(c,"UP_OFF")==0) {
    if (tiltCont && tiltDir==+1) tiltCont = false;
  } else if (strcmp(c,"DOWN_ON")==0) {
    tiltCont = true;
    tiltDir  = -1;
  } else if (strcmp(c,"DOWN_OFF")==0) {
    if (tiltCont && tiltDir==-1) tiltCont = false;
  }

  // angle move "(pan,tilt)"
  else if (c[0]=='(') {
    // parse "(pan,tilt)"
    char tmp[BUF_SZ];
    strncpy(tmp, c+1, BUF_SZ);
    char* sep = strchr(tmp, ',');
    if (!sep) return;
    *sep = '\0';
    float dPan  = atof(tmp);
    float dTilt = atof(sep+1);

    // step calculation
    const float panStepDeg  = 360.0f / (200.0f * 8.0f * 13.76f);
    const float tiltStepDeg = 360.0f / (200.0f * 8.0f * 50.0f);
    long stepsPan  = lroundf(fabs(dPan)  / panStepDeg);
    long stepsTilt = lroundf(fabs(dTilt) / tiltStepDeg);

    // pan move
    digitalWrite(PAN_DIR_PIN, (dPan>=0 ? HIGH : LOW));
    for (long i=0; i<stepsPan; i++) stepPin(PAN_STEP_PIN);

    // tilt move
    digitalWrite(TILT_DIR_PIN, (dTilt>=0 ? HIGH : LOW));
    for (long i=0; i<stepsTilt; i++) stepPin(TILT_STEP_PIN);
  }
  // anything else ignored
}

void setup() {
  Serial.begin(115200);
  pinMode(PAN_STEP_PIN,   OUTPUT);
  pinMode(PAN_DIR_PIN,    OUTPUT);
  pinMode(TILT_STEP_PIN,  OUTPUT);
  pinMode(TILT_DIR_PIN,   OUTPUT);
}

void loop() {
  // build command buffer until newline
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      buf[idx] = '\0';
      handleCmd(buf);
      idx = 0;
    } else if (c != '\r' && idx < BUF_SZ-1) {
      buf[idx++] = c;
    }
  }

  // continuous stepping
  if (panCont) {
    digitalWrite(PAN_DIR_PIN, panDir>0 ? HIGH : LOW);
    stepPin(PAN_STEP_PIN);
  }
  if (tiltCont) {
    digitalWrite(TILT_DIR_PIN, tiltDir>0 ? HIGH : LOW);
    stepPin(TILT_STEP_PIN);
  }
}
