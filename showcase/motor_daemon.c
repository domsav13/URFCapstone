// motor_daemon.c
// Compile with:
//   gcc -std=c99 -O2 -Wall -o motor_daemon motor_daemon.c

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>    
#include <sys/stat.h>  
#include <termios.h>
#include <errno.h>
#include <time.h>      

#define SERIAL_PORT "/dev/ttyACM0"
#define FIFO_PATH   "/tmp/arduino_cmd"

typedef speed_t Baud;

// Open the serial port and return its file descriptor
int openSerialPort(const char* port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("open serial port");
    }
    return fd;
}

// Configure serial port: 115200 8N1, no flow control
int configureSerialPort(int fd, Baud baud) {
    struct termios t;
    memset(&t, 0, sizeof t);
    if (tcgetattr(fd, &t) != 0) {
        perror("tcgetattr");
        return -1;
    }
    cfsetospeed(&t, baud);
    cfsetispeed(&t, baud);

    t.c_cflag = (t.c_cflag & ~CSIZE) | CS8;
    t.c_iflag &= ~IGNBRK;
    t.c_lflag = 0;
    t.c_oflag = 0;
    t.c_cc[VMIN]  = 0;
    t.c_cc[VTIME] = 5;

    t.c_iflag &= ~(IXON | IXOFF | IXANY);
    t.c_cflag |= (CLOCAL | CREAD);
    t.c_cflag &= ~(PARENB | PARODD);
    t.c_cflag &= ~CSTOPB;
    #ifdef CRTSCTS
        t.c_cflag &= ~CRTSCTS;
    #endif

    if (tcsetattr(fd, TCSANOW, &t) != 0) {
        perror("tcsetattr");
        return -1;
    }
    return 0;
}

// Send a null-terminated command string (with newline) to the Arduino
int sendCmd(int fd, const char* s) {
    char buf[80];
    int len = snprintf(buf, sizeof buf, "%s\n", s);
    if (len < 0 || len >= (int)sizeof buf) {
        fprintf(stderr, "Command too long\n");
        return -1;
    }
    if (write(fd, buf, len) < 0) {
        perror("write to serial");
        return -1;
    }
    return 0;
}

int main(void) {
    // Open and configure serial port
    int sd = openSerialPort(SERIAL_PORT);
    if (sd < 0) return 1;
    if (configureSerialPort(sd, B115200) < 0) {
        close(sd);
        return 1;
    }

    // Ensure FIFO exists
    if (access(FIFO_PATH, F_OK) == -1) {
        if (mkfifo(FIFO_PATH, 0666) != 0) {
            perror("mkfifo");
            close(sd);
            return 1;
        }
    }

    // Open FIFO read-end (blocks until writer opens) and a dummy write-end
    int fr = open(FIFO_PATH, O_RDONLY);
    int fw = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
    if (fr < 0) {
        perror("open FIFO for read");
        close(sd);
        return 1;
    }

    printf("motor_daemon: listening on %s\n", FIFO_PATH);

    char cmd[64];
    struct timespec pause = { .tv_sec = 0, .tv_nsec = 50000000L }; // 50 ms

    // Main loop: read from FIFO, log, forward to Arduino
    while (1) {
        ssize_t n = read(fr, cmd, sizeof(cmd) - 1);
        if (n > 0) {
            cmd[n] = '\0';
            if (cmd[n - 1] == '\n') cmd[n - 1] = '\0';
            printf("motor_daemon: got '%s' → sending to Arduino\n", cmd);
            if (sendCmd(sd, cmd) < 0) {
                fprintf(stderr, "motor_daemon: error sending '%s'\n", cmd);
            }
        }
        else if (n < 0) {
            perror("read FIFO");
        }
        nanosleep(&pause, NULL);
    }

    // Cleanup (never reached)
    close(fw);
    close(fr);
    close(sd);
    return 0;
}
