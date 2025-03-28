#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#define SERIAL_PORT "/dev/ttyACM0"  // Adjust if necessary

// Open the serial port.
int openSerialPort(const char* port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("Error opening serial port");
    }
    return fd;
}

// Configure the serial port.
int configureSerialPort(int fd, speed_t baudRate) {
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        perror("Error from tcgetattr");
        return -1;
    }
    
    cfsetospeed(&tty, baudRate);
    cfsetispeed(&tty, baudRate);
    
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;  // 8-bit characters
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;                             // No canonical processing
    tty.c_oflag = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 5;                         // 0.5 sec timeout
    
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("Error from tcsetattr");
        return -1;
    }
    return 0;
}

// Wait for Arduino's "Ready" message.
int waitForArduino(int fd) {
    char buf[64];
    int total_read = 0;
    while (total_read < 5) {  // "Ready" is 5 characters
        int n = read(fd, buf + total_read, sizeof(buf) - total_read - 1);
        if (n > 0) {
            total_read += n;
            buf[total_read] = '\0';
            if (strstr(buf, "Ready")) {
                printf("Arduino is ready: %s\n", buf);
                return 0;
            }
        } else {
            usleep(100000); // 100 ms
        }
    }
    fprintf(stderr, "Timeout waiting for Arduino to be ready.\n");
    return -1;
}

int main(void) {
    int fd = openSerialPort(SERIAL_PORT);
    if (fd < 0) return 1;
    
    if (configureSerialPort(fd, B115200) < 0) {
        close(fd);
        return 1;
    }
    
    if (waitForArduino(fd) < 0) {
        close(fd);
        return 1;
    }
    
    char command[32];
    printf("Enter command (angle value 0-360, or 'CW', 'CCW', 'STOP'): ");
    if (fgets(command, sizeof(command), stdin) == NULL) {
        fprintf(stderr, "Error reading input.\n");
        close(fd);
        return 1;
    }
    
    // Remove newline if present.
    size_t len = strlen(command);
    if (len > 0 && command[len - 1] == '\n') {
        command[len - 1] = '\0';
    }
    
    int n = write(fd, command, strlen(command));
    if (n < 0) {
        perror("Error writing to serial port");
    } else {
        printf("Sent to Arduino: %s\n", command);
    }
    
    close(fd);
    return 0;
}
