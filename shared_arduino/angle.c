#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

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

    // ✅ Flush the buffer
    tcflush(serial, TCIOFLUSH);

    return serial;
}

// Send target position to Arduino
void sendTargetPosition(int serial, float target) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%.2f\n", target);
    write(serial, buffer, strlen(buffer));
    printf("Sent target position: %.2f degrees\n", target);
}

int main() {
    int serial = openSerialPort();
    float targetPosition;

    while (1) {
        printf("Enter target position (0-360 degrees, or -1 to exit): ");
        scanf("%f", &targetPosition);

        if (targetPosition < 0 || targetPosition > 360) {
            printf("Exiting program.\n");
            break;
        }

        // ✅ Send target to Arduino
        sendTargetPosition(serial, targetPosition);
    }

    close(serial);
    return 0;
}
