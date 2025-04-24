// motor_daemon.c
// Compile with:
//   gcc -std=c99 -O2 -Wall -o motor_daemon motor_daemon.c

#define _POSIX_C_SOURCE 200809L   // for nanosleep()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>     // open(), read(), write(), close()
#include <sys/stat.h>   // mkfifo()
#include <termios.h>
#include <errno.h>
#include <time.h>       // nanosleep()

#define SERIAL_PORT  "/dev/ttyACM0"
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
        perror("tcgetattr");
        return -1;
    }
    cfsetospeed(&tty, baudRate);
    cfsetispeed(&tty, baudRate);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;   // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                      // disable break processing
    tty.c_lflag = 0;                             // no signaling chars, no echo
    tty.c_oflag = 0;                             // no remapping, no delays
    tty.c_cc[VMIN]  = 0;                         // read doesn't block
    tty.c_cc[VTIME] = 5;                         // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);      // shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL | CREAD);             // ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD);           // no parity
    tty.c_cflag &= ~CSTOPB;                      // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;                     // no hardware flow control

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        return -1;
    }
    return 0;
}

// Wait for "Ready" message from Arduino, polling every 100 ms
int waitForArduino(int fd) {
    char buf[64];
    int total_read = 0;
    struct timespec req = { .tv_sec = 0, .tv_nsec = 100000000L }; // 100 ms

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
            if (n < 0) perror("read serial");
            nanosleep(&req, NULL);
        }
    }
    fprintf(stderr, "Timeout waiting for Arduino\n");
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
    if (write(fd, serialCmd, len) < 0) {
        perror("Error writing to serial port");
        return -1;
    }
    return 0;
}

int main(void) {
    // Open and configure the serial port
    int serial_fd = openSerialPort(SERIAL_PORT);
    if (serial_fd < 0) return 1;
    if (configureSerialPort(serial_fd, B115200) < 0) {
        close(serial_fd);
        return 1;
    }

    // Wait for Arduino startup message
    if (waitForArduino(serial_fd) < 0) {
        close(serial_fd);
        return 1;
    }

    // Create FIFO if it doesn't exist
    if (access(FIFO_PATH, F_OK) == -1) {
        if (mkfifo(FIFO_PATH, 0666) != 0) {
            perror("Error creating FIFO");
            close(serial_fd);
            return 1;
        }
    }

    // Open FIFO for reading (blocks until a writer appears)
    int fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd < 0) {
        perror("Error opening FIFO for read");
        close(serial_fd);
        return 1;
    }
    // Open dummy write end so the read side never sees EOF
    int dummy_fd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);

    printf("Daemon is running, waiting for commands on %s\n", FIFO_PATH);

    char command[64];
    struct timespec pause = { .tv_sec = 0, .tv_nsec = 100000000L }; // 100 ms

    // Main loop: read from FIFO and forward to Arduino
    while (1) {
        ssize_t n = read(fifo_fd, command, sizeof(command) - 1);
        if (n > 0) {
            command[n] = '\0';
            // Strip trailing newline if present
            if (n > 0 && command[n - 1] == '\n') {
                command[n - 1] = '\0';
            }
            printf("Received command: '%s'\n", command);
            if (sendCommand(serial_fd, command) == 0) {
                printf("Sent to Arduino: '%s'\n", command);
            }
        } else if (n < 0) {
            perror("Error reading from FIFO");
        }
        // Brief pause before checking again
        nanosleep(&pause, NULL);
    }

    // Cleanup (never reached)
    close(dummy_fd);
    close(fifo_fd);
    close(serial_fd);
    return 0;
}
