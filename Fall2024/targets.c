#include "shared_memory.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

double random_double(double min, double max) {
        return min + ((double)rand() / RAND_MAX) * (max-min);
}

int main() {
        int navID = shmget((key_t)123, 1024, 0666);
        NavigationPos *Nav_data = (NavigationPos *)shmat(navID, NULL, 0);

        int targID = shmget((key_t)12, 1024, 0666 | IPC_CREAT);
        TargetPos *Targ_data = (TargetPos *)shmat(targID, NULL, 0);

        while (Nav_data->termination == 0) {
                srand(time(NULL));
                int random_number = rand() % 100;
                if (random_number < 15) {
                        double Lat = random_double(-75.0,0.0); // random region of Target latitude
                        double Lon = random_double(-15.0, 45.0); // random region of Target longitude
                        double Alt = random_double(0,Nav_data->BoatAlt); // random altitude between sea level and boat altitude
                        printf("NEW TARGET - Lat: %.4f, Lon: %.4f, Alt: %.4f \n",Lat,Lon,Alt);
                        Targ_data->TargetLat = Lat;
                        Targ_data->TargetLon = Lon;
                        Targ_data->TargetAlt = Alt;
                        Targ_data->present = true;
                } else {
                        printf("No new target.\n");
                }
                printf("----------------------------------------------------\n");
                sleep(1);
        }
}
