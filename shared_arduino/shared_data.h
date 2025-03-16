#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <stdint.h>
#include <stdbool.h>

#define SHM_KEY 1234 // Shared memory key
#define SHM_SIZE (sizeof(shared_data_t))

typedef struct {
    uint16_t t_on;
    uint16_t t_off;
    uint16_t position;
    float velocity;
    bool newData;
} shared_data_t;

#endif
