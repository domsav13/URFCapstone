#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
        double pitch;
        double roll;
        double yaw;

} IMUAttitude;



typedef struct {
        double BoatLat;
        double BoatLon;
        double BoatAlt;
        double BoatPitch;
        double BoatRoll;
        double BoatYaw;
        int termination;


} NavigationPos;


typedef struct {
        double TargetLat;
        double TargetLon;
        double TargetAlt;
        bool present;

} TargetPos;