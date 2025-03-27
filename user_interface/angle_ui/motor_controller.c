#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#define SERIAL_PORT "/dev/ttyACM0"  // Adjust if necessary

// Open the serial port
int openSerialPort(const char* port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("Error opening serial port");
    }
    return fd;
}

// Configure the serial port with the given baud rate (B115200 in this case)
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
    tty.c_iflag &= ~IGNBRK;                      // disable break processing
    tty.c_lflag = 0;                             // no signaling chars, no echo, no canonical processing
    tty.c_oflag = 0;                             // no remapping, no delays
    tty.c_cc[VMIN]  = 0;                         // non-blocking read
    tty.c_cc[VTIME] = 5;                         // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);        // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);               // ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD);             // shut off parity
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("Error from tcsetattr");
        return -1;
    }
    return 0;
}

int main(void) {
    // Open and configure the serial port
    int fd = openSerialPort(SERIAL_PORT);
    if (fd < 0) {
        return 1;
    }
    
    if (configureSerialPort(fd, B115200) < 0) {
        close(fd);
        return 1;
    }
    
    // Simple text-based UI: prompt user for an angle
    float angle;
    printf("Enter an angle (0-360 degrees): ");
    if (scanf("%f", &angle) != 1) {
        fprintf(stderr, "Invalid input.\n");
        close(fd);
        return 1;
    }
    
    // Validate the input angle
    if (angle < 0 || angle > 360) {
        fprintf(stderr, "Angle must be between 0 and 360 degrees.\n");
        close(fd);
        return 1;
    }
    
    // Prepare the string to send (including a newline)
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f\n", angle);
    
    // Send the angle to the Arduino
    int n = write(fd, buf, strlen(buf));
    if (n < 0) {
        perror("Error writing to serial port");
    } else {
        printf("Sent to Arduino: %s", buf);
    }
    
    // Clean up
    close(fd);
    return 0;
}
