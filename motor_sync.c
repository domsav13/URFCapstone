#include <wiringPi.h>
#include <stdio.h>
#include <unistd.h>

#define MOTOR1 0 // This is GPIO 17
#define MOTOR2 1 // This is GPIO 18
#define PULSE_TIME 100000 // 100 ms per ON/OFF cycle
#define TOTAL_TIME 15000000

int main() {

	if (wiringPiSetupGpio() == -1) {
		printf("Failed to initialize WiringPi! \n");
		return 1;
	}

	pinMode(MOTOR1, OUTPUT);
	pinMode(MOTOR2, OUTPUT);

	printf("Starting motor control loop...\n");
	int elapsed_time = 0;

	while (elapsed_time < TOTAL_TIME) {
		digitalWrite(MOTOR1, HIGH);
		digitalWrite(MOTOR2, HIGH);
		usleep(PULSE_TIME); // 500 ms
		elapsed_time += PULSE_TIME;

		digitalWrite(MOTOR1, LOW);
		digitalWrite(MOTOR2, LOW);
		usleep(PULSE_TIME); // 500 ms
		elapsed_time += PULSE_TIME;
	}
	digitalWrite(MOTOR1, LOW);
	digitalWrite(MOTOR2, LOW);
	printf("Loop complete. Motors off. \n");
	return 0;

}
