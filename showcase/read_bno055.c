// read_bno055.c
// Compile with:
//   gcc -std=c99 -O2 -Wall -o read_bno055 read_bno055.c -lm

#define _GNU_SOURCE       // for M_PI
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <math.h>

#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif

#define I2C_BUS            "/dev/i2c-1"
#define BNO055_ADDR        0x28
#define REG_OPR_MODE       0x3D
#define OPR_MODE_CONFIG    0x00
#define OPR_MODE_NDOF      0x0C
#define REG_EULER_H_LSB    0x1A

static void msleep(unsigned int msec) {
    usleep(msec * 1000);
}

static int16_t read_raw_euler(int file, int16_t *roll, int16_t *pitch) {
    uint8_t reg = REG_EULER_H_LSB;
    if (write(file, &reg, 1) != 1) return -1;
    uint8_t d[6];
    if (read(file, d, 6) != 6) return -1;
    int16_t heading = (int16_t)((d[1]<<8) | d[0]);
    *roll  = (int16_t)((d[3]<<8) | d[2]);
    *pitch = (int16_t)((d[5]<<8) | d[4]);
    return heading;
}

int main(void) {
    int file = open(I2C_BUS, O_RDWR);
    if (file < 0) { perror("Open I2C"); return EXIT_FAILURE; }
    if (ioctl(file, I2C_SLAVE, BNO055_ADDR) < 0) {
        perror("I2C_SLAVE"); close(file); return EXIT_FAILURE;
    }

    // CONFIG mode
    uint8_t buf[2] = {REG_OPR_MODE, OPR_MODE_CONFIG};
    if (write(file, buf, 2)!=2) { perror("CONFIG"); close(file); return EXIT_FAILURE; }
    msleep(25);

    // NDOF mode
    buf[1] = OPR_MODE_NDOF;
    if (write(file, buf, 2)!=2) { perror("NDOF"); close(file); return EXIT_FAILURE; }
    msleep(20);

    // initial zero-offset read (ignored here)
    int16_t r0, p0;
    if (read_raw_euler(file, &r0, &p0) < 0) {
        fprintf(stderr, "Init read failed\n");
        close(file);
        return EXIT_FAILURE;
    }

    // Continuous print loop
    while (1) {
        int16_t raw_roll, raw_pitch;
        int16_t raw_heading = read_raw_euler(file, &raw_roll, &raw_pitch);
        if (raw_heading < 0) { perror("Read"); break; }

        float yaw   = raw_heading / 16.0f;
        float roll  = raw_roll    / 16.0f;
        float pitch = raw_pitch   / 16.0f;

        // Print CSV: yaw,roll,pitch
        printf("%.2f,%.2f,%.2f\n", yaw, roll, pitch);
        fflush(stdout);

        msleep(500);
    }

    close(file);
    return EXIT_SUCCESS;
}
