#include <stdio.h>
#include <wiringPi.h>

#define MOTOR1 17
#define MOTOR2 18

// Compile: gcc motor_sync2.c motor_sync2 -l wiringPi
// Execute: sudo ./motor_sync2

int main (void)
{
  wiringPiSetupGpio();
  pinMode(MOTOR1, OUTPUT);
  pinMode(MOTOR2, OUTPUT);

  int x;
  for (x = 0; x < 5; x++) {
    digitalWrite(MOTOR1, 1);
    digitalWrite(MOTOR2, 1);
    delay(500);
    digitalWrite(MOTOR1, 0);
    digitalWrite(MOTOR2, 0);
    delay(500);
  }

  return 0;

}
