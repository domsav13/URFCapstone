// read_bno055.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_BUS            "/dev/i2c-1"
#define BNO055_ADDR        0x28      // ADR pin low; change to 0x29 if ADR is high

// BNO055 internal registers
#define REG_OPR_MODE       0x3D
#define OPR_MODE_CONFIG    0x00
#define OPR_MODE_NDOF      0x0C

#define REG_EULER_H_LSB    0x1A      // 6 bytes: heading LSB/MSB, roll LSB/MSB, pitch LSB/MSB

// Delay helper (ms)
static void msleep(unsigned int msec) {
    usleep(msec * 1000);
}

int main(void) {
    int file;
    if ((file = open(I2C_BUS, O_RDONLY | O_RDWR)) < 0) {
        perror("Opening I2C bus");
        return EXIT_FAILURE;
    }

    if (ioctl(file, I2C_SLAVE, BNO055_ADDR) < 0) {
        perror("Setting I2C address");
        close(file);
        return EXIT_FAILURE;
    }

    // 1) Switch to CONFIG mode
    uint8_t buf[2] = { REG_OPR_MODE, OPR_MODE_CONFIG };
    if (write(file, buf, 2) != 2) {
        perror("Write CONFIG mode");
        close(file);
        return EXIT_FAILURE;
    }
    msleep(25);

    // 2) Switch to NDOF fusion mode
    buf[1] = OPR_MODE_NDOF;
    if (write(file, buf, 2) != 2) {
        perror("Write NDOF mode");
        close(file);
        return EXIT_FAILURE;
    }
    msleep(20);

    printf(" BNO055 NDOF mode\n");
    printf("Reading Euler angles (press Ctrl+C to quit)\n");

    while (1) {
        // Tell the chip we want to read from the Euler registers
        uint8_t reg = REG_EULER_H_LSB;
        if (write(file, &reg, 1) != 1) {
            perror("Selecting Euler reg");
            break;
        }

        // Read 6 bytes: H_LSB, H_MSB, R_LSB, R_MSB, P_LSB, P_MSB
        uint8_t data[6];
        if (read(file, data, 6) != 6) {
            perror("Reading Euler data");
            break;
        }

        // Convert to 16‑bit signed integers (little endian)
        int16_t heading = (int16_t)((data[1] << 8) | data[0]);
        int16_t roll    = (int16_t)((data[3] << 8) | data[2]);
        int16_t pitch   = (int16_t)((data[5] << 8) | data[4]);

        // According to datasheet, unit is 1/16° per LSB
        float h = heading / 16.0f;
        float r = roll    / 16.0f;
        float p = pitch   / 16.0f;

        printf("\rHeading: %7.2f°   Roll: %7.2f°   Pitch: %7.2f°   ",
               h, r, p);
        fflush(stdout);

        msleep(100);
    }

    close(file);
    return EXIT_SUCCESS;
}
