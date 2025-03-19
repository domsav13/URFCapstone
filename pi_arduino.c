#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define SERIAL_PORT "/dev/ttyACM0"
#define BAUD_RATE B115200

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
    printf("Sent: %s\n", command);
}

int main() {
    int serial = openSerialPort();
    char input[16];

    while (1) {
        printf("Enter command (ON, OFF, EXIT): ");
        scanf("%s", input);

        if (strcmp(input, "EXIT") == 0) {
            break;
        } else if (strcmp(input, "ON") == 0 || strcmp(input, "OFF") == 0) {
            sendCommand(serial, input);
        } else {
            printf("Invalid command. Use ON or OFF.\n");
        }
    }

    close(serial);
    printf("Exiting program.\n");
    return 0;
}
