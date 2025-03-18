#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/select.h>
#include <pthread.h>
#include "shared_data.h"

#define SERIAL_PORT "/dev/ttyACM0"
#define BAUD_RATE 115200

int shm_fd;
shared_data_t *shared_data;
FILE *serial;
pthread_mutex_t console_lock = PTHREAD_MUTEX_INITIALIZER;
volatile int suppress_encoder_output = 0;  // Flag to disable encoder output

// ✅ Read encoder data and store in shared memory
void *read_encoder_data(void *arg) {
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
                    shared_data->t_on = t_on;
                    shared_data->t_off = t_off;
                    shared_data->position = position;

                    static uint16_t last_position = 0;
                    shared_data->velocity = (position - last_position) * 0.01;
                    shared_data->newData = true;

                    last_position = position;

                    // ✅ Suppress encoder output when taking user input
                    if (!suppress_encoder_output) {
                        pthread_mutex_lock(&console_lock);
                        printf("High Time: %d, Low Time: %d, Position: %d, Velocity: %.2f\n",
                               shared_data->t_on, shared_data->t_off, shared_data->position, shared_data->velocity);
                        pthread_mutex_unlock(&console_lock);
                    }
                }
            }
        }

        usleep(10000); // Poll every 10 ms
    }
}

// ✅ Handle user input and send commands to Arduino
void *handle_user_input(void *arg) {
    int targetAngle;

    while (1) {
        // ✅ Suppress encoder output while waiting for user input
        suppress_encoder_output = 1;

        pthread_mutex_lock(&console_lock);
        printf("\nEnter target angle (0–360) or -1 to exit: ");
        pthread_mutex_unlock(&console_lock);

        if (scanf("%d", &targetAngle) == 1) {
            if (targetAngle == -1) {
                pthread_mutex_lock(&console_lock);
                printf("Exiting input loop.\n");
                pthread_mutex_unlock(&console_lock);
                break;
            }
            if (targetAngle >= 0 && targetAngle <= 360) {
                fprintf(serial, "%d\n", targetAngle);
                fflush(serial);

                pthread_mutex_lock(&console_lock);
                printf("Sent target angle: %d\n", targetAngle);
                pthread_mutex_unlock(&console_lock);
            } else {
                pthread_mutex_lock(&console_lock);
                printf("Invalid input. Must be between 0 and 360.\n");
                pthread_mutex_unlock(&console_lock);
            }
        } else {
            // ✅ Clear input buffer on invalid input
            pthread_mutex_lock(&console_lock);
            printf("Invalid input.\n");
            pthread_mutex_unlock(&console_lock);
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
        }

        // ✅ Resume encoder output after user input is handled
        suppress_encoder_output = 0;
    }

    return NULL;
}

int main() {
    // ✅ Open shared memory
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

    serial = fopen(SERIAL_PORT, "w+");
    if (serial == NULL) {
        perror("Failed to open serial port");
        return 1;
    }

    pthread_t read_thread, input_thread;

    // ✅ Start thread to read encoder data
    pthread_create(&read_thread, NULL, read_encoder_data, NULL);

    // ✅ Start thread to handle user input
    pthread_create(&input_thread, NULL, handle_user_input, NULL);

    // ✅ Keep both threads alive
    pthread_join(read_thread, NULL);
    pthread_join(input_thread, NULL);

    fclose(serial);
    shm_unlink("/shm_motor");

    return 0;
}
