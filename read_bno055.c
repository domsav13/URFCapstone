// read_bno055.c
// Compile: gcc -std=c99 -O2 -Wall -o read_bno055 read_bno055.c -lm
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
    int file = open(I2C_BUS, O_RDWR | O_RDONLY);
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

    // initial zero
    int16_t r0, p0;
    int16_t h0 = read_raw_euler(file, &r0, &p0);
    if (h0<0) { fprintf(stderr,"Init read failed\n"); close(file); return EXIT_FAILURE; }
    float off_h = h0/16.0f;
    float off_r = r0/16.0f;
    float off_p = p0/16.0f;

    printf("Zero offsets → H:%.2f°, R:%.2f°, P:%.2f°\n\n", off_h, off_r, off_p);
    printf("   H        R        P    Pan      Tilt\n");
    printf("------------------------------------------------\n");

    while (1) {
        int16_t r, p;
        int16_t h = read_raw_euler(file, &r, &p);
        if (h<0) { perror("Read"); break; }

        // convert to degrees
        float yaw   = h/16.0f  - off_h;
        float roll  = r/16.0f  - off_r;
        float pitch = p/16.0f  - off_p;

        // to radians
        float ay = yaw   * (M_PI/180.0f);
        float ap = pitch * (M_PI/180.0f);
        float ar = roll  * (M_PI/180.0f);

        // pan & tilt
        float pan_rad  = acosf( sinf(ay)*sinf(ap)*sinf(ar)
                              + cosf(ay)*cosf(ar) );
        float tilt_rad = acosf( cosf(ap)*cosf(ar) );

        float pan_deg  = pan_rad  * (180.0f/M_PI);
        float tilt_deg = tilt_rad * (180.0f/M_PI);

        // print
        printf("%7.2f  %7.2f  %7.2f  %7.2f  %7.2f\n",
               yaw, roll, pitch, pan_deg, tilt_deg);

        msleep(100);
    }

    close(file);
    return EXIT_SUCCESS;
}
