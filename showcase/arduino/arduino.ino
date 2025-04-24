// pan_tilt_continuous_and_angle.ino
// – CW/CCW toggles tilt motor (pins 23/25)
// – UP/DOWN toggles pan motor   (pins 22/24)
// – “(pan,tilt)” moves both axes together (positive pan = CCW, negative = CW; positive tilt = UP, negative = DOWN)
// – no Serial prints for smooth operation

#include <Arduino.h>

#define PAN_STEP_PIN     22
#define PAN_DIR_PIN      24
#define TILT_STEP_PIN    23
#define TILT_DIR_PIN     25
#define STEPS_PER_REV    200
#define MICROSTEPS       8
#define PAN_GEAR_REDUCT  13.76f
#define TILT_GEAR_REDUCT 50.0f
#define PAN_DEG_PER_STEP  (360.0f/(STEPS_PER_REV*MICROSTEPS*PAN_GEAR_REDUCT))
#define TILT_DEG_PER_STEP (360.0f/(STEPS_PER_REV*MICROSTEPS*TILT_GEAR_REDUCT))
#define STEP_DELAY_US    90

bool panCont   = false;
int  panDir    = 1;    // +1 = UP, -1 = DOWN

bool tiltCont  = false;
int  tiltDir   = 1;    // +1 = CCW, -1 = CW

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
  digitalWrite(PAN_DIR_PIN, panDir>0 ? HIGH : LOW);
  stepPin(PAN_STEP_PIN);
}

void continuousTiltStep() {
  digitalWrite(TILT_DIR_PIN, tiltDir>0 ? HIGH : LOW);
  stepPin(TILT_STEP_PIN);
}

void movePanTilt(float panDeg, float tiltDeg) {
  long panSteps  = lroundf(fabs(panDeg)  / PAN_DEG_PER_STEP);
  long tiltSteps = lroundf(fabs(tiltDeg) / TILT_DEG_PER_STEP);

  digitalWrite(PAN_DIR_PIN,  panDeg  >=0 ? HIGH : LOW);
  digitalWrite(TILT_DIR_PIN, tiltDeg>=0 ? HIGH : LOW);

  long dx = panSteps, dy = tiltSteps;
  long err = dx - dy;

  while (dx>0 || dy>0) {
    long e2 = err*2;
    if (dx>0 && e2>-dy) {
      stepPin(PAN_STEP_PIN);
      err -= dy; dx--;
    }
    if (dy>0 && e2<dx) {
      stepPin(TILT_STEP_PIN);
      err += dx; dy--;
    }
  }
}

void handleCmd(const char* c) {
  // TILT: CW / CCW
  if (strcmp(c,"CW")==0) {
    if (!tiltCont)       { tiltCont=true;  tiltDir=-1; }
    else if (tiltDir<0)  { tiltCont=false;           }
    else                 { tiltDir  = -1;            }
  }
  else if (strcmp(c,"CCW")==0) {
    if (!tiltCont)       { tiltCont=true;  tiltDir=+1; }
    else if (tiltDir>0)  { tiltCont=false;           }
    else                 { tiltDir  = +1;            }
  }
  // PAN: UP / DOWN
  else if (strcmp(c,"UP")==0) {
    if (!panCont)        { panCont=true;   panDir=+1; }
    else if (panDir>0)   { panCont=false;            }
    else                 { panDir   = +1;            }
  }
  else if (strcmp(c,"DOWN")==0) {
    if (!panCont)        { panCont=true;   panDir=-1; }
    else if (panDir<0)   { panCont=false;            }
    else                 { panDir   = -1;            }
  }
  // Vector move "(pan,tilt)"
  else if (c[0]=='(') {
    char tmp[BUF_SZ];
    strncpy(tmp, c+1, BUF_SZ);
    char* sep = strchr(tmp,',');
    if (!sep) return;
    *sep = '\0';
    float pd = atof(tmp), td = atof(sep+1);
    movePanTilt(pd, td);
  }
  // ignore other
}

void setup(){
  Serial.begin(115200);
  pinMode(PAN_STEP_PIN,   OUTPUT);
  pinMode(PAN_DIR_PIN,    OUTPUT);
  pinMode(TILT_STEP_PIN,  OUTPUT);
  pinMode(TILT_DIR_PIN,   OUTPUT);
}

void loop(){
  while (Serial.available()) {
    char c = Serial.read();
    if (c=='\n') {
      buf[idx]=0;
      handleCmd(buf);
      idx=0;
    } else if (c!='\r' && idx<BUF_SZ-1) {
      buf[idx++]=c;
    }
  }

  if (panCont)   continuousPanStep();
  if (tiltCont)  continuousTiltStep();
}
