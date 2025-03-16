#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <stdint.h>
#include <stdbool.h>

#define SHM_KEY 1234
#define SHM_SIZE (sizeof(shared_data_t))

// Structure for shared data (aligned to avoid memory corruption)
typedef struct __attribute__((aligned(4))) {
    uint16_t t_on;
    uint16_t t_off;
    uint16_t position;
    float velocity;
    volatile bool newData;
} shared_data_t;

#endif
