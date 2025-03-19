#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <math.h>
#include <sys/select.h>

#define SERIAL_PORT "/dev/ttyACM0"
#define BAUD_RATE B115200

// Open the serial port
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

    // ✅ Flush any junk in the buffer
    tcflush(serial, TCIOFLUSH);

    // ✅ Short delay to let Arduino initialize
    usleep(500000); // 500ms

    return serial;
}

// Send target position to Arduino
void sendTargetPosition(int serial, float target) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%.2f\n", target);
    write(serial, buffer, strlen(buffer));
    printf("Sent target position: %.2f degrees\n", target);
}

// Read position directly using `read()`
float readPosition(int serial) {
    char buffer[64];
    float position = -1;
    int index = 0;
    char c;

    // ✅ Read character-by-character until newline is received
    while (index < sizeof(buffer) - 1) {
        int n = read(serial, &c, 1);
        if (n > 0) {
            if (c == '\n') {
                buffer[index] = '\0'; // End the string
                break;
            } else {
                buffer[index++] = c;
            }
        }
    }

    if (index > 0) {
        // ✅ Try to parse the float value
        if (sscanf(buffer, "%f", &position) == 1) {
            printf("Current Position: %.2f degrees\n", position);
        } else {
            fprintf(stderr, "Failed to parse position from: '%s'\n", buffer);
        }
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

        sendTargetPosition(serial, targetPosition);

        float currentPosition;
        while (1) {
            currentPosition = readPosition(serial);

            // ✅ Check if position is valid and within tolerance
            if (currentPosition >= 0 && fabs(currentPosition - targetPosition) <= 1.0) {
                printf("Target position reached!\n");
                break;
            }

            usleep(50000); // ✅ Poll every 50ms (same as Arduino throttle time)
        }
    }

    close(serial);
    printf("Exiting program.\n");
    return 0;
}
