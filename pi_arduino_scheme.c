#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define SERIAL_PORT "/dev/ttyACM0"
#define BAUD_RATE B115200
#define TOLERANCE 50 // Encoder tolerance for matching

// Open serial port
int openSerialPort() {
    int serial = open(SERIAL_PORT, O_RDWR | O_NOCTTY);
    if (serial == -1) {
        perror("Failed to open serial port");
        exit(1);
    }

    struct termios options;
    tcgetattr(serial, &options);
    cfsetispeed(&options, BAUD_RATE);
    cfsetospeed(&options, BAUD_RATE);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    tcsetattr(serial, TCSANOW, &options);

    tcflush(serial, TCIOFLUSH);

    return serial;
}

// Send command to Arduino
void sendCommand(int serial, const char *command) {
    write(serial, command, strlen(command));
    write(serial, "\n", 1);
    tcdrain(serial);
    printf("Sent: %s\n", command);
}

// Read encoder position from Arduino
int readEncoderPosition(int serial) {
    char buffer[32];
    int n = read(serial, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        int position = atoi(buffer);
        return position;
    }
    return -1;
}

// Convert angle to encoder position
int angleToPosition(float angle) {
    return (int)((angle / 360.0) * 1024);
}

int main() {
    int serial = openSerialPort();
    float targetAngle;
    int targetPosition;
    int encoderPosition;

    printf("Enter target angle (0 to 360): ");
    scanf("%f", &targetAngle);

    if (targetAngle < 0 || targetAngle > 360) {
        printf("Invalid angle. Must be between 0 and 360.\n");
        close(serial);
        return 1;
    }

    targetPosition = angleToPosition(targetAngle);
    printf("Target encoder position: %d\n", targetPosition);

    // Send ON command to start motor
    sendCommand(serial, "ON");

    while (1) {
        encoderPosition = readEncoderPosition(serial);
        if (encoderPosition >= 0) {
            printf("Current encoder position: %d\n", encoderPosition);

            // If encoder is within tolerance of target, stop motor
            if (abs(encoderPosition - targetPosition) <= TOLERANCE) {
                printf("Target reached! Stopping motor.\n");
                sendCommand(serial, "OFF");
                break;
            }
        }

        usleep(100000); // 100 ms delay
    }

    close(serial);
    printf("Exiting program.\n");
    return 0;
}
