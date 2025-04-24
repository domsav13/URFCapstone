// motor_daemon.c
// Compile with:
//   gcc -std=c99 -O2 -Wall -o motor_daemon motor_daemon.c
#define _POSIX_C_SOURCE 200809L   // expose usleep() in unistd.h
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>   // for usleep()
#include <sys/stat.h> // for mkfifo()
#include <termios.h>
#include <errno.h>


#define SERIAL_PORT  "/dev/ttyACM0"
#define FIFO_PATH    "/tmp/arduino_cmd"

typedef speed_t Baud;

int openSerialPort(const char* port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) perror("Error opening serial port");
    return fd;
}

int configureSerialPort(int fd, Baud baudRate) {
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        return -1;
    }
    cfsetospeed(&tty, baudRate);
    cfsetispeed(&tty, baudRate);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 5;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        return -1;
    }
    return 0;
}

int waitForArduino(int fd) {
    char buf[64];
    int total = 0;
    while (total < 5) {
        int n = read(fd, buf + total, sizeof(buf) - total - 1);
        if (n > 0) {
            total += n;
            buf[total] = '\0';
            if (strstr(buf, "Ready")) {
                printf("Arduino is ready: %s\n", buf);
                return 0;
            }
        } else {
            usleep(100000);
        }
    }
    fprintf(stderr, "Timeout waiting for Arduino\n");
    return -1;
}

int sendCommand(int fd, const char* cmd) {
    char out[80];
    int len = snprintf(out, sizeof out, "%s\n", cmd);
    if (len < 0 || len >= (int)sizeof out) {
        fprintf(stderr, "Command too long\n");
        return -1;
    }
    if (write(fd, out, len) < 0) {
        perror("write");
        return -1;
    }
    return 0;
}

int main(void) {
    int serial_fd = openSerialPort(SERIAL_PORT);
    if (serial_fd < 0) return 1;
    if (configureSerialPort(serial_fd, B115200) < 0) {
        close(serial_fd);
        return 1;
    }
    if (waitForArduino(serial_fd) < 0) {
        close(serial_fd);
        return 1;
    }

    // ensure /tmp/arduino_cmd exists
    if (access(FIFO_PATH, F_OK) == -1) {
        if (mkfifo(FIFO_PATH, 0666) != 0) {
            perror("mkfifo");
            close(serial_fd);
            return 1;
        }
    }

    // open read end (blocks until at least one writer appears)
    int fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd < 0) {
        perror("open FIFO for read");
        close(serial_fd);
        return 1;
    }
    // open dummy write end so we never see EOF
    int dummy_fd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);

    printf("Daemon running, waiting for commands on %s\n", FIFO_PATH);

    char cmd[64];
    while (1) {
        ssize_t n = read(fifo_fd, cmd, sizeof(cmd) - 1);
        if (n > 0) {
            cmd[n] = '\0';
            // strip trailing newline if any
            if (cmd[n-1] == '\n') cmd[n-1] = '\0';
            printf("Received: '%s'\n", cmd);
            if (sendCommand(serial_fd, cmd) == 0)
                printf("Forwarded to Arduino\n");
        } else if (n < 0) {
            perror("read FIFO");
        }
        // on n==0 we never hit EOF because dummy_fd stays open
    }

    // never reached
    close(dummy_fd);
    close(fifo_fd);
    close(serial_fd);
    return 0;
}
