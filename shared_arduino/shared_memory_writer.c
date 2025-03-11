#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "shared_data.h"

#define SERIAL_PORT "/dev/ttyACM0"
#define BAUD_RATE 115200

int main() {
    int shm_fd;
    shared_data_t *shared_data;

    // Create shared memory segment
    shm_fd = shm_open("/shm_motor", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Shared memory creation failed");
        return 1;
    }

    // Resize the shared memory segment
    ftruncate(shm_fd, SHM_SIZE);

    // Map the shared memory
    shared_data = (shared_data_t *)mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("Shared memory mapping failed");
        return 1;
    }

    // Open serial port
    FILE *serial = fopen(SERIAL_PORT, "r");
    if (serial == NULL) {
        perror("Failed to open serial port");
        return 1;
    }

    printf("Reading data from Arduino...\n");

    char buffer[64];
    while (fgets(buffer, sizeof(buffer), serial) != NULL) {
        int t_on, t_off, position;

        // Parse incoming data
        if (sscanf(buffer, "%d,%d,%d", &t_on, &t_off, &position) == 3) {
            shared_data->t_on = t_on;
            shared_data->t_off = t_off;
            shared_data->position = position;

            // Compute velocity (basic example)
            static uint16_t last_position = 0;
            float velocity = (position - last_position) * 0.01; // Example scale factor
            shared_data->velocity = velocity;

            shared_data->newData = true;

            last_position = position;

            printf("High Time: %d, Low Time: %d, Position: %d, Velocity: %.2f\n",
                   shared_data->t_on, shared_data->t_off, shared_data->position, shared_data->velocity);
        }
    }

    fclose(serial);
    shm_unlink("/shm_motor");

    return 0;
}
