#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/file.h>
#include <errno.h>

#define SERIAL_PORT "/dev/ttyACM0"  // Adjust as needed

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

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;  // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                      // disable break processing
    tty.c_lflag = 0;                             // no signaling chars, no echo, no canonical processing
    tty.c_oflag = 0;                             // no remapping, no delays
    tty.c_cc[VMIN]  = 0;                         // non-blocking read
    tty.c_cc[VTIME] = 5;                         // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);        // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);               // ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD);             // no parity
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("Error from tcsetattr");
        return -1;
    }
    return 0;
}

int main(void) {
    // Acquire a file lock to ensure only one instance accesses the serial port.
    int lock_fd = open("/tmp/motor_controller.lock", O_CREAT | O_RDWR, 0666);
    if (lock_fd < 0) {
        perror("Error opening lock file");
        return 1;
    }
    if (flock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
        perror("Unable to acquire lock, another instance might be running");
        close(lock_fd);
        return 1;
    }
    
    int fd = openSerialPort(SERIAL_PORT);
    if (fd < 0) {
        flock(lock_fd, LOCK_UN);
        close(lock_fd);
        return 1;
    }
    
    if (configureSerialPort(fd, B115200) < 0) {
        close(fd);
        flock(lock_fd, LOCK_UN);
        close(lock_fd);
        return 1;
    }
    
    // Delay to allow the Arduino to reset after the serial port is opened.
    sleep(2);
    
    float angle;
    printf("Enter an angle (0-360 degrees): ");
    if (scanf("%f", &angle) != 1) {
        fprintf(stderr, "Invalid input.\n");
        close(fd);
        flock(lock_fd, LOCK_UN);
        close(lock_fd);
        return 1;
    }
    
    if (angle < 0 || angle > 360) {
        fprintf(stderr, "Angle must be between 0 and 360 degrees.\n");
        close(fd);
        flock(lock_fd, LOCK_UN);
        close(lock_fd);
        return 1;
    }
    
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f\n", angle);
    
    int n = write(fd, buf, strlen(buf));
    if (n < 0) {
        perror("Error writing to serial port");
    } else {
        printf("Sent to Arduino: %s", buf);
    }
    
    close(fd);
    // Release the file lock.
    flock(lock_fd, LOCK_UN);
    close(lock_fd);
    return 0;
}
