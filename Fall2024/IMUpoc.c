#include "shared_memory.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
        int shmid = shmget((key_t)1234, 1024, 0666 | IPC_CREAT);
        IMUAttitude *shared_data = (IMUAttitude *)shmat(shmid, NULL, 0);
        int navID = shmget((key_t)123, 1024, 0666);
        NavigationPos *NavPos = (NavigationPos *)shmat(navID, NULL, 0);

        double attitude[3] = {0,0,0};

        while(NavPos->termination == 0) {
                double random_chance = (double)rand() / RAND_MAX;
                if (random_chance < 1.0 / 3.0) {
                        int index_to_change = rand() % 3;
                        attitude[index_to_change] += ((double)rand() / RAND_MAX - 0.5) * 2.0;
                }
                shared_data->pitch = attitude[0];
                shared_data->roll = attitude[1];
                shared_data->yaw = attitude[2];
                printf("Pitch: %1f, Roll: %1f, Yaw: %1f \n",attitude[0],attitude[1],attitude[2]);
                printf("---------------------------------------------\n");

                sleep(1);
        }
}
