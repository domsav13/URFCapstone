#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define STEP_PIN 21          // GPIO 21 (Pulse)
#define STEP_ANGLE 1.8       // Step angle per full step
#define MICROSTEPS 8         // Microstepping mode (set via DIP switches on MBC15081)
#define DEG_PER_STEP (STEP_ANGLE / MICROSTEPS)

// Global variable for step delay
int STEP_DELAY = 1000;  // Default delay in microseconds

// Setup GPIO
void setup() {
    wiringPiSetupGpio();  // Use Broadcom GPIO numbering
    pinMode(STEP_PIN, OUTPUT);
}

// Function to perform steps
void stepMotor(int steps) {
    for (int i = 0; i < steps; i++) {
        digitalWrite(STEP_PIN, HIGH);
        usleep(STEP_DELAY);
        digitalWrite(STEP_PIN, LOW);
        usleep(STEP_DELAY);
    }
}

// Function to rotate the stepper motor by a given angle
void rotateMotor(float angle) {
    int steps = (int)(angle / DEG_PER_STEP);  // Convert angle to step count
    printf("Rotating motor by %.2f degrees (%d steps) with delay of %d microseconds\n", angle, steps, STEP_DELAY);
    stepMotor(steps);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <angle> <step_delay>\n", argv[0]);
        return 1;
    }

    float angle = atof(argv[1]);  // Read angle from command-line argument
    STEP_DELAY = atoi(argv[2]);   // Read step delay from command-line argument

    // Validate step delay
    if (STEP_DELAY < 1) {
        printf("⚠️ WARNING: Minimum step delay is 1 microsecond. Setting to 1.\n");
        STEP_DELAY = 1;
    }

    if (angle == 0) {
        printf("Exiting program.\n");
        return 0;
    }

    setup();  // Initialize GPIO
    rotateMotor(angle);

    return 0;
}

