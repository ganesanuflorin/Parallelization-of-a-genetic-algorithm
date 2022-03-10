#ifndef THREAD_STR_H
#define THREAD_STR_H

#include <pthread.h>
#include "sack_object.h"

typedef struct threadStr {
    int id;
    pthread_barrier_t *barrier;
    int P;
    const sack_object *objects;
    int object_count;
    int generations_count;
    int sack_capacity;
    individual *current_generation;
    individual *next_generation;
} threadStr;

#endif