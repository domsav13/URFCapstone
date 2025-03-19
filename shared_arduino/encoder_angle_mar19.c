#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <math.h>

#define SERIAL_PORT "/dev/ttyACM0"
#define BAUD_RATE B115200

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

    // ✅ Flush the buffer to remove any stale data
    tcflush(serial, TCIOFLUSH);

    return serial;
}

void sendTargetPosition(int serial, float target) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%.2f\n", target);
    write(serial, buffer, strlen(buffer));
    printf("Sent target position: %.2f degrees\n", target);
}

float readPosition(int serial) {
    char buffer[64];
    float position = -1;

    // ✅ Use fgets to read a complete line
    if (fgets(buffer, sizeof(buffer), fdopen(serial, "r")) != NULL) {
        // ✅ Try to parse the angle value
        if (sscanf(buffer, "%f", &position) == 1) {
            printf("Current Position: %.2f degrees\n", position);
        } else {
            printf("Failed to parse position from: '%s'\n", buffer);
        }
    } else {
        printf("No data received.\n");
    }

    return position;
}

int main() {
    int serial = openSerialPort();
    float targetPosition;

    while (1) {
        printf("Enter target position (0-360 degrees, or -1 to exit): ");
        scanf("%f", &targetPosition);

        if (targetPosition == -1) break;

        // ✅ Send target position to Arduino
        sendTargetPosition(serial, targetPosition);

        float currentPosition;
        while (1) {
            currentPosition = readPosition(serial);
            if (fabs(currentPosition - targetPosition) <= 1.0) {
                printf("Target position reached!\n");
                break;
            }
            usleep(10000); // ✅ Poll every 10ms to avoid serial flooding
        }
    }

    close(serial);
    printf("Exiting program.\n");
    return 0;
}
