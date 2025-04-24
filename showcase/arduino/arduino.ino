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
  // PAN: CCW_ON / CCW_OFF / CW_ON / CW_OFF
  if (strcmp(c,"CCW_ON")==0) {
    panCont = true; panDir = +1;
  } else if (strcmp(c,"CCW_OFF")==0) {
    if (panCont && panDir==+1) panCont=false;
  } else if (strcmp(c,"CW_ON")==0) {
    panCont = true; panDir = -1;
  } else if (strcmp(c,"CW_OFF")==0) {
    if (panCont && panDir==-1) panCont=false;
  }
  // TILT: UP_ON / UP_OFF / DOWN_ON / DOWN_OFF
  else if (strcmp(c,"UP_ON")==0) {
    tiltCont = true; tiltDir = +1;
  } else if (strcmp(c,"UP_OFF")==0) {
    if (tiltCont && tiltDir==+1) tiltCont=false;
  } else if (strcmp(c,"DOWN_ON")==0) {
    tiltCont = true; tiltDir = -1;
  } else if (strcmp(c,"DOWN_OFF")==0) {
    if (tiltCont && tiltDir==-1) tiltCont=false;
  }
  // angle move "(pan,tilt)"
  else if (c[0]=='(') {
    // expect "(pan,tilt)"
    char tmp[BUF_SZ];
    strncpy(tmp, c+1, BUF_SZ);
    char* p = strchr(tmp,',');
    if (!p) return;
    *p = '\0';
    float pan = atof(tmp);
    float tilt= atof(p+1);
    // clamp tilt, pan unconstrained
    movePanTilt(pan, tilt);
  }
  // otherwise ignore
}

void setup(){
  Serial.begin(115200);
  pinMode(PAN_STEP_PIN,   OUTPUT);
  pinMode(PAN_DIR_PIN,    OUTPUT);
  pinMode(TILT_STEP_PIN,  OUTPUT);
  pinMode(TILT_DIR_PIN,   OUTPUT);
}

void loop(){
  while(Serial.available()){
    char c = Serial.read();
    if(c=='\n'){
      buf[idx]=0;
      handleCmd(buf);
      idx=0;
    } else if(c!='\r' && idx<BUF_SZ-1){
      buf[idx++]=c;
    }
  }
  if(panCont)   continuousPanStep();
  if(tiltCont)  continuousTiltStep();
}
