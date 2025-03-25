#define CLOCK_PIN 22   // Clock signal pin
#define DIRECTION_PIN 23 // Optional direction pin, not used in this setup

volatile uint16_t t_on = 0;   // Stores high-time (PWM on time)
volatile uint16_t t_off = 0;  // Stores low-time (PWM off time)
volatile bool newData = false;

volatile uint16_t position = 0;       // Current encoder position
volatile uint16_t targetPosition = 0; // Target encoder position
volatile bool motorRun = false;
volatile bool targetReached = false;

void setup() {
  // Set motor pins as outputs
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DIRECTION_PIN, OUTPUT);
  digitalWrite(CLOCK_PIN, LOW);
  digitalWrite(DIRECTION_PIN, LOW);

  // Initialize serial communication
  Serial.begin(115200);

  // Set up external interrupt for encoder readings
  EICRA = 0b00001111; // INT0 and INT1 on rising and falling edge
  EIMSK = 0b00000011; // Enable INT0 and INT1

  // Set up Timer1 for motor control signal (100µs interval)
  TCCR1A = 0b00000000;  
  TCCR1B = 0b00001010;  // CTC mode, Prescaler = 8
  OCR1A = 999;          // Compare value for 100µs intervals
  TIMSK1 = 0b00000010;  // Enable Timer1 Compare A Match interrupt

  // Set up Timer5 for encoder signal
  TCCR5A = 0b00000000;
  TCCR5B = 0b11000010; // Input capture on falling edge, prescaler = 8
  TIMSK5 = 0b00100001; // Enable input capture and overflow interrupts

  sei(); // Enable global interrupts
}

void loop() {
  encoderPosition(); // Continuously update encoder position


  if (motorRun && !targetReached) {
    if (abs(position - targetPosition) <= 5) { // ±1 degree tolerance
      motorRun = false;
      targetReached = true;
      stopMotor(); // Stop motor when target is reached
    }
  }

  // Handle serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "ON") {
      motorRun = true;
      targetReached = false;
    } 
    else if (command == "OFF") {
      motorRun = false;
      stopMotor();
    } 
    else if (command.startsWith("TARGET ")) {
      int target = command.substring(7).toInt();
      if (target >= 0 && target <= 1024) {
        targetPosition = target;
        motorRun = true;
        targetReached = false;
        Serial.print("New Target: ");
        Serial.println(targetPosition);
      }
    }
  }
}

// ==========================
// === Timer Interrupts ===
// ==========================

// Timer1 interrupt for motor control signal
ISR(TIMER1_COMPA_vect) {
  if (motorRun) {
    PINA = 0b00000011; // Toggle CLOCK_PIN and DIRECTION_PIN
  }
}

// Timer5 interrupt for encoder pulse measurement
ISR(TIMER5_CAPT_vect) {
  static uint16_t lastCapture = 0;
  static bool measuringOnTime = true;
  uint16_t currentCapture = ICR5;

  if (measuringOnTime) {
    t_on = currentCapture - lastCapture;
    TCCR5B ^= (1 << ICES5); // Toggle to capture rising edge next
  } else {
    t_off = currentCapture - lastCapture;
    newData = true;
    TCCR5B ^= (1 << ICES5); // Toggle to capture falling edge next
  }

  measuringOnTime = !measuringOnTime;
  lastCapture = currentCapture;
}

// =============================
// === Functions ===
// =============================

void encoderPosition() {
  if (newData) {
    cli(); // Disable interrupts while reading
    uint16_t highTime = t_on;
    uint16_t lowTime = t_off;
    newData = false;
    sei(); // Re-enable interrupts

    // Convert high and low time to encoder position (0-1024 range)
    float t_on_us = highTime * 0.5;
    float t_off_us = lowTime * 0.5;

    float x = ((t_on_us * 1026) / (t_on_us + t_off_us)) - 1;

    position = (x <= 1022) ? x : 1023;
    if ((position>0)&&(position<1024)){
      Serial.println(position);
    }
  }
}

void stopMotor() {
  digitalWrite(CLOCK_PIN, LOW);
  digitalWrite(DIRECTION_PIN, LOW);
}
