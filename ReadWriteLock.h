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

int rwlock_init(ReadWriteLock *rw);

int rwlock_destroy(ReadWriteLock *rw);

int rwlock_before_write(ReadWriteLock *rw);

int rwlock_after_write(ReadWriteLock *rw);

int rwlock_before_read(ReadWriteLock *rw);

int rwlock_after_read(ReadWriteLock *rw);
