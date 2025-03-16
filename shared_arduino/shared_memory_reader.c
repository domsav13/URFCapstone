#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include "shared_data.h"

int shm_fd;
shared_data_t *shared_data;

void cleanup(int sig) {
    printf("\nClosing shared memory...\n");
    if (shared_data != MAP_FAILED) {
        munmap(shared_data, SHM_SIZE);
    }
    if (shm_fd != -1) {
        close(shm_fd);
    }
    shm_unlink("/shm_motor");
    printf("Shared memory closed.\n");
    _exit(0);
}

int main() {
    signal(SIGINT, cleanup);

    // ✅ Open shared memory in READ/WRITE mode (not just read-only)
    shm_fd = shm_open("/shm_motor", O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Failed to open shared memory");
        return 1;
    }

    shared_data = (shared_data_t *)mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("Failed to map shared memory");
        return 1;
    }

    printf("Reading shared memory...\n");

    while (1) {
        if (shared_data->newData) {
            // ✅ Create local copy to avoid corruption
            shared_data_t data = *shared_data;

            printf("High Time: %d, Low Time: %d, Position: %d, Velocity: %.2f\n",
                   data.t_on, data.t_off, data.position, data.velocity);

            // ✅ Reset only after successful read
            shared_data->newData = false;
        }

        usleep(10000); // Poll every 10 ms
    }

    return 0;
}
