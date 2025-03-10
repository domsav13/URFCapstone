#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define STEP_PIN 21          // GPIO 21 (Pulse)
#define STEP_ANGLE 1.8       // Step angle per full step
#define MICROSTEPS 8         // Microstepping mode (set via DIP switches on MBC15081)
#define DEG_PER_STEP (STEP_ANGLE / MICROSTEPS)

volatile int running = 0;    // Motor running state
volatile int STEP_DELAY = 1000; // Default step delay in microseconds

// Handle termination signals (CTRL+C)
void handleSignal(int signal) {
    running = 0;
}

// Setup GPIO
void setup() {
    wiringPiSetupGpio();  // Use Broadcom GPIO numbering
    pinMode(STEP_PIN, OUTPUT);
}

// Function to step the motor
void stepMotor() {
    digitalWrite(STEP_PIN, HIGH);
    usleep(STEP_DELAY);
    digitalWrite(STEP_PIN, LOW);
    usleep(STEP_DELAY);
}

// Function to rotate the motor continuously while running is true
void rotateMotor() {
    while (running) {
        stepMotor();
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <step_delay>\n", argv[0]);
        return 1;
    }

    STEP_DELAY = atoi(argv[1]);
    if (STEP_DELAY < 1) {
        printf("⚠️ WARNING: Minimum step delay is 1 microsecond. Setting to 1.\n");
        STEP_DELAY = 1;
    }

    // Handle termination signals
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);

    setup();

    printf("Starting motor with step delay of %d microseconds.\n", STEP_DELAY);
    running = 1;

    rotateMotor();

    printf("Motor stopped.\n");
    return 0;
}
