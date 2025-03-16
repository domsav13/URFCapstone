#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/select.h>
#include "shared_data.h"

#define SERIAL_PORT "/dev/ttyACM0"
#define BAUD_RATE 115200

int main() {
    int shm_fd;
    shared_data_t *shared_data;

    shm_fd = shm_open("/shm_motor", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Shared memory creation failed");
        return 1;
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("Failed to set shared memory size");
        return 1;
    }

    shared_data = (shared_data_t *)mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("Failed to map shared memory");
        return 1;
    }

    FILE *serial = fopen(SERIAL_PORT, "r");
    if (serial == NULL) {
        perror("Failed to open serial port");
        return 1;
    }

    printf("Reading data from Arduino...\n");

    char buffer[128];
    while (1) {
        fd_set set;
        struct timeval timeout;

        FD_ZERO(&set);
        FD_SET(fileno(serial), &set);

        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; // 10 ms

        if (select(fileno(serial) + 1, &set, NULL, NULL, &timeout) > 0) {
            int n = read(fileno(serial), buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';

                int t_on, t_off, position;
                if (sscanf(buffer, "%d,%d,%d", &t_on, &t_off, &position) == 3) {
                    // Always write (don't wait for reader)
                    shared_data->t_on = t_on;
                    shared_data->t_off = t_off;
                    shared_data->position = position;

                    static uint16_t last_position = 0;
                    shared_data->velocity = (position - last_position) * 0.01;
                    shared_data->newData = true;

                    last_position = position;

                    printf("High Time: %d, Low Time: %d, Position: %d, Velocity: %.2f\n",
                           shared_data->t_on, shared_data->t_off, shared_data->position, shared_data->velocity);
                }
            }
        }

        usleep(10000); // Poll every 10 ms
    }

    fclose(serial);
    shm_unlink("/shm_motor");

    return 0;
}
