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

    tcflush(serial, TCIOFLUSH);

    return serial;
}

void sendCommand(int serial, const char *command) {
    write(serial, command, strlen(command));
    write(serial, "\n", 1);
    printf("Sent: %s\n", command);
}

float readCurrentAngle(int serial) {
    char buffer[64];
    float angle = -1;

    int n = read(serial, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        if (sscanf(buffer, "Angle: %f", &angle) == 1) {
            printf("Current Angle: %.2f degrees\n", angle);
        }
    }

    return angle;
}

int main() {
    int serial = openSerialPort();
    float targetPosition;

    while (1) {
        printf("Enter target position (0-360 degrees, -1 to exit): ");
        scanf("%f", &targetPosition);

        if (targetPosition < 0 || targetPosition > 360) {
            printf("Exiting program.\n");
            sendCommand(serial, "OFF");
            break;
        }

        sendCommand(serial, "ON");
        usleep(100000);  // Small delay to stabilize

        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%.2f", targetPosition);
        sendCommand(serial, buffer);

        while (1) {
            float currentAngle = readCurrentAngle(serial);
            if (currentAngle >= 0 && fabs(currentAngle - targetPosition) <= 1.0) {
                printf("Target position reached!\n");
                sendCommand(serial, "OFF");
                break;
            }
            usleep(100000); // ✅ Poll every 100ms
        }
    }

    close(serial);
    return 0;
}
