#pragma once

#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t readers;
    pthread_cond_t writers;
    int n_reading, n_writing;
    int n_readers_waiting, n_writers_waiting;
    int change;
} ReadWriteLock;

// todo: sygnatury ale najpierw je zmienic
