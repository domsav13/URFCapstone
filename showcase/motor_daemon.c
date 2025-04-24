// motor_daemon.c
// Compile with:
//   gcc -std=c99 -O2 -Wall -o motor_daemon motor_daemon.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>     // open(), read(), write(), close(), usleep()
#include <sys/stat.h>   // mkfifo()
#include <termios.h>
#include <errno.h>

#define SERIAL_PORT  "/dev/ttyACM0"  // Adjust if necessary
#define FIFO_PATH    "/tmp/arduino_cmd"

typedef speed_t Baud;

// Open the serial port and return file descriptor
int openSerialPort(const char* port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("Error opening serial port");
    }
    return fd;
}

// Configure serial port parameters (115200 8N1, no flow control)
int configureSerialPort(int fd, Baud baudRate) {
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        perror("Error from tcgetattr");
        return -1;
    }
    cfsetospeed(&tty, baudRate);
    cfsetispeed(&tty, baudRate);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;   // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                      // disable break processing
    tty.c_lflag = 0;                             // no signaling chars, no echo, no canonical
    tty.c_oflag = 0;                             // no remapping, no delays
    tty.c_cc[VMIN]  = 0;                         // read doesn't block
    tty.c_cc[VTIME] = 5;                         // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);      // shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL | CREAD);             // ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD);           // shut off parity
    tty.c_cflag &= ~CSTOPB;                      // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;                     // no hardware flow control

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("Error from tcsetattr");
        return -1;
    }
    return 0;
}

// Wait for "Ready" message from Arduino
int waitForArduino(int fd) {
    char buf[64];
    int total_read = 0;
    while (total_read < 5) {
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

// Send a command string to Arduino, appending newline
int sendCommand(int fd, const char* cmd) {
    char serialCmd[80];
    int len = snprintf(serialCmd, sizeof(serialCmd), "%s\n", cmd);
    if (len < 0 || len >= (int)sizeof(serialCmd)) {
        fprintf(stderr, "Command too long: '%s'\n", cmd);
        return -1;
    }
    int w = write(fd, serialCmd, len);
    if (w < 0) {
        perror("Error writing to serial port");
        return -1;
    }
    return 0;
}

int main(void) {
    // Open and configure the serial port
    int fd = openSerialPort(SERIAL_PORT);
    if (fd < 0) return 1;
    if (configureSerialPort(fd, B115200) < 0) {
        close(fd);
        return 1;
    }

    // Wait for Arduino startup message
    if (waitForArduino(fd) < 0) {
        close(fd);
        return 1;
    }

    // Create FIFO if it doesn't exist
    if (access(FIFO_PATH, F_OK) == -1) {
        if (mkfifo(FIFO_PATH, 0666) != 0) {
            perror("Error creating FIFO");
            close(fd);
            return 1;
        }
    }

    // Open FIFO for reading commands
    int fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd < 0) {
        perror("Error opening FIFO");
        close(fd);
        return 1;
    }

    char command[64];
    printf("Daemon is running. Waiting for commands...\n");

    // Main loop: read from FIFO and forward to Arduino
    while (1) {
        int n = read(fifo_fd, command, sizeof(command) - 1);
        if (n > 0) {
            command[n] = '\0';
            // Strip trailing newline
            size_t len = strlen(command);
            if (len > 0 && command[len - 1] == '\n') {
                command[len - 1] = '\0';
            }

            printf("Received command: '%s'\n", command);
            if (sendCommand(fd, command) == 0) {
                printf("Sent to Arduino: '%s'\n", command);
            }
        } else if (n < 0) {
            perror("Error reading from FIFO");
        }
        usleep(100000);  // 100 ms delay between reads
    }

    close(fifo_fd);
    close(fd);
    return 0;
}
