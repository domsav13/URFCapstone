#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "shared_data.h"

int main() {
    int shm_fd;
    shared_data_t *shared_data;

    // Open shared memory
    shm_fd = shm_open("/shm_motor", O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("Failed to open shared memory");
        return 1;
    }

    shared_data = (shared_data_t *)mmap(0, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);

    while (1) {
        if (shared_data->newData) {
            printf("High Time: %d, Low Time: %d, Position: %d, Velocity: %.2f\n",
                   shared_data->t_on, shared_data->t_off, shared_data->position, shared_data->velocity);
            shared_data->newData = false;
        }
        usleep(10000); // 10 ms delay
    }

    return 0;
}
