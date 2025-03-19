#define ENCODER_PIN 5

volatile uint16_t t_on = 0;  
volatile uint16_t t_off = 0; 
volatile bool newData = false;

unsigned long lastSendTime = 0;

void setup() 
{
    Serial.begin(115200);

    // ✅ Wait for serial connection to stabilize
    while (!Serial) {
        delay(10);
    }

    pinMode(ENCODER_PIN, INPUT);

    // ✅ Timer3 setup (Pin 5 → ICP3)
    TCCR3A = 0b00000000;
    TCCR3B = 0b11000010; // Falling edge, prescaler = 8
    TCCR3C = 0b00000000;
    TIMSK3 = 0b00100001; // Enable input capture interrupt + overflow interrupt

    delay(100);
    Serial.println("Encoder on Timer3 ready...");
}

ISR(TIMER3_CAPT_vect) {
    static uint16_t lastCapture = 0;
    static bool measuringOnTime = true;
    uint16_t currentCapture = ICR3;

    if (measuringOnTime) {
        t_on = currentCapture - lastCapture;
        TCCR3B ^= (1 << ICES3); // Switch to capture rising edge next
    } 
    else {
        t_off = currentCapture - lastCapture;
        newData = true;
        TCCR3B ^= (1 << ICES3); // Switch to capture falling edge next
    }

    measuringOnTime = !measuringOnTime;
    lastCapture = currentCapture;
}

float readEncoderAngle() {
    if (newData) {
        cli();  
        uint16_t highTime = t_on;
        uint16_t lowTime = t_off;
        newData = false;
        sei();

        float t_on_us = highTime * 0.5;
        float t_off_us = lowTime * 0.5;

        float x = ((t_on_us * 1026) / (t_on_us + t_off_us)) - 1;
        uint16_t position = (x <= 1022) ? x : 1023;

        // ✅ Convert to angle (0-360 degrees)
        float angle = (position * 360.0) / 1024.0;
        return angle;
    }
    return -1;
}

void loop() 
{
    if (millis() - lastSendTime >= 1000) {
        lastSendTime = millis();

        float angle = readEncoderAngle();
        if (angle >= 0) {
            Serial.print("Angle: ");
            Serial.println(angle);
        }
    }
}
