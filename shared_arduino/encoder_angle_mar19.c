#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define SERIAL_PORT "/dev/ttyACM0"
#define BAUD_RATE B115200

// Function to open serial port
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

    return serial;
}

// Function to send target position to Arduino
void sendTargetPosition(int serial, int target) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d\n", target);
    write(serial, buffer, strlen(buffer));
    printf("Sent target position: %d\n", target);
}

// Function to read current position from Arduino
int readPosition(int serial) {
    char buffer[64];
    int position = -1;

    int n = read(serial, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        int t_on, t_off;
        if (sscanf(buffer, "%d,%d,%d", &t_on, &t_off, &position) == 3) {
            printf("High Time: %d, Low Time: %d, Position: %d\n", t_on, t_off, position);
        }
    }

    return position;
}

int main() {
    int serial = openSerialPort();
    int targetPosition;

    while (1) {
        printf("Enter target position (0-360 degrees, or -1 to exit): ");
        scanf("%d", &targetPosition);

        if (targetPosition == -1) break;

        // Send target position to Arduino
        sendTargetPosition(serial, targetPosition);

        // Continuously monitor until the target position is reached
        int currentPosition;
        while (1) {
            currentPosition = readPosition(serial);
            if (currentPosition == targetPosition) {
                printf("Target position reached!\n");
                break;
            }
            usleep(10000); // Poll every 10ms
        }
    }

    close(serial);
    printf("Exiting program.\n");
    return 0;
}
