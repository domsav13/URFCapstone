#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/shm.h>
#include<string.h>
#include "shared_memory.h"
#include <math.h>
#include <stdbool.h>

double pan_angle(double lat1,double lon1,double lat2,double lon2) {
        // Let "1" denote boat
        double dy = (lat2-lat1)*100000; // meters
        double dx = (lon2-lon1)*100000; // meters
        double angle = atan2(dx,dy);

        return angle*(180.0 / M_PI);
}

double tilt_angle(double lat1,double lon1,double alt1,double lat2,double lon2,double alt2) {
        double dz = alt1-alt2; // meters
        double dy = (lat2-lat1)*100000; // meters
        double dx = (lon2-lon1)*100000; // meters
        double r = sqrt(pow(dy,2)+pow(dx,2)); // meters
        double angle = atan2(dz,r);

        return angle*(180.0 / M_PI);
}

int main() {
        double tilt = 0.0;
        double pan = 0.0;
        printf("Initializing Pan and Tilt rotations to 0.0 \n");
        int imuID = shmget((key_t)1234, 1024, 0666);
        IMUAttitude *IMU_data = (IMUAttitude *)shmat(imuID, NULL, 0);

        int navID = shmget((key_t)123, 1024, 0666);
        NavigationPos *Nav_data = (NavigationPos *)shmat(navID, NULL, 0);

        int targID = shmget((key_t)12, 1024, 0666);
        TargetPos *Targ_data = (TargetPos *)shmat(targID, NULL, 0);

        while(Nav_data->termination == 0) {
                double sumPitch = IMU_data->pitch + Nav_data->BoatPitch;
                double sumRoll = IMU_data->roll + Nav_data->BoatRoll;
                double sumYaw = IMU_data->yaw + Nav_data->BoatYaw;
                double pan_rotation = 0.0;
                double tilt_rotation = 0.0;

                if(Targ_data->present == true) {
                        pan_rotation = pan_angle(Nav_data->BoatLat,Nav_data->BoatLon,Targ_data->TargetLat,Targ_data->TargetLon);
                        tilt_rotation = tilt_angle(Nav_data->BoatLat,Nav_data->BoatLon,Nav_data->BoatAlt,
                                Targ_data->TargetLat,Targ_data->TargetLon,Targ_data->TargetAlt);
                }
                printf("Tracking Target... Pan rotation: %.2f deg, Tilt rotation: %.6f deg\n",pan_rotation,tilt_rotation);
                printf("Measuring drift... Total pitch: %.2f, Total roll: %.2f, Total yaw %.2f\n",sumPitch,sumRoll,sumYaw);
                printf("Stabilizing spotlight... Tilt rotation: %.2f deg, Pan rotation:%.2f deg\n",sumPitch,sumYaw);
                printf("----------------------------------------------------------------------------------\n");

                sleep(1);
        }
}
