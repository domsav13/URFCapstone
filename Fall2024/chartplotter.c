#include "shared_memory.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
        int navID = shmget((key_t)123, 1024, 0666 | IPC_CREAT);
        NavigationPos *shared_data = (NavigationPos *)shmat(navID,NULL,0);

        // Holding longitude and altitude constant
        double lon = -60.0;
        double alt = 10.0; // Meters
        shared_data->BoatLon = lon;
        shared_data->BoatAlt = alt;

        double BoatAtt[3] = {0,0,0};

        double LatStart = 30.000;
        double LatEnd = 30.050;
        double step = 0.001;
        shared_data->termination = 0;

        for (double i = LatStart; i <= LatEnd; i += step) {
                printf("Latitude: %.4f, Longitude: %.4f, Altitude: %.4f \n",i,lon,alt);
                shared_data->BoatLon = i;

                // Generating random changes in boat attitude
                double random_chance = (double)rand() / RAND_MAX;
                if (random_chance < 1.0 / 4.0) {
                        int index_to_change = rand() % 3;
                        BoatAtt[index_to_change] += ((double)rand() / RAND_MAX - 0.5) *2.0;
                }
                shared_data->BoatPitch = BoatAtt[0];
                shared_data->BoatRoll = BoatAtt[1];
                shared_data->BoatYaw = BoatAtt[2];
                printf("Boat Pitch: %.2f, Boat Roll: %.2f, Boat Yaw: %.2f \n" , BoatAtt[0],BoatAtt[1],BoatAtt[2]);
                printf("---------------------------------------------------- \n");
                sleep(1);
        }
        shared_data->termination = 1;
}
