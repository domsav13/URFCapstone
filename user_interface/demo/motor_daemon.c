#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

#define SERIAL_PORT "/dev/ttyACM0"  // Adjust if necessary
#define FIFO_PATH "/tmp/arduino_cmd"

int openSerialPort(const char* port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("Error opening serial port");
    }
    return fd;
}

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
    // Open and configure the serial port.
    int fd = openSerialPort(SERIAL_PORT);
    if (fd < 0) return 1;
    
    if (configureSerialPort(fd, B115200) < 0) {
        close(fd);
        return 1;
    }
    
    // Wait for the Arduino to send its "Ready" message.
    if (waitForArduino(fd) < 0) {
        close(fd);
        return 1;
    }

    // Create the FIFO if it does not exist.
    if (access(FIFO_PATH, F_OK) == -1) {
        if (mkfifo(FIFO_PATH, 0666) != 0) {
            perror("Error creating FIFO");
            close(fd);
            return 1;
        }
    }
    
    // Open the FIFO for reading.
    int fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd < 0) {
        perror("Error opening FIFO");
        close(fd);
        return 1;
    }
    
    char command[64];
    printf("Daemon is running. Waiting for commands...\n");
    
    // Main loop: read commands from the FIFO and send to Arduino.
    while (1) {
        int n = read(fifo_fd, command, sizeof(command) - 1);
        if (n > 0) {
            command[n] = '\0';
            // Remove any trailing newline.
            size_t len = strlen(command);
            if (len > 0 && command[len - 1] == '\n') {
                command[len - 1] = '\0';
            }
            printf("Received command: %s\n", command);
            
            // Write the command to the serial port.
            int w = write(fd, command, strlen(command));
            if (w < 0) {
                perror("Error writing to serial port");
            } else {
                printf("Sent to Arduino: %s\n", command);
            }
        } else if (n < 0) {
            perror("Error reading from FIFO");
        }
        usleep(100000);  // Optional: a short delay between reads.
    }
    
    close(fifo_fd);
    close(fd);
    return 0;
}
