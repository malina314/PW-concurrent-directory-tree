#include "ReadWriteLock.h"

#include <pthread.h>

int rwlock_init(ReadWriteLock *rw) {
    int err;
    if ((err = pthread_mutex_init(&rw->mutex, 0)) != 0)
        return err;
    if ((err = pthread_cond_init(&rw->readers, 0)) != 0)
        return err;
    if ((err = pthread_cond_init(&rw->writers, 0)) != 0)
        return err;
    rw->n_readers_waiting = rw->n_writers_waiting = rw->n_reading = rw->n_writing = rw->change = 0;
    return 0;
}

int rwlock_destroy(ReadWriteLock *rw) {
    int err;
    if ((err = pthread_cond_destroy(&rw->readers)) != 0)
        return err;
    if ((err = pthread_cond_destroy(&rw->writers)) != 0)
        return err;
    if ((err = pthread_mutex_destroy(&rw->mutex)) != 0)
        return err;
    return 0;
}

int rwlock_before_write(ReadWriteLock *rw) {
    int err;
    if ((err = pthread_mutex_lock(&rw->mutex)) != 0)
        return err;

    rw->n_writers_waiting++;

    while (rw->n_reading > 0 || rw->n_writing > 0)
        if ((err = pthread_cond_wait(&rw->writers, &rw->mutex)) != 0)
            return err;

    rw->n_writers_waiting--;
    rw->n_writing++;

    if ((err = pthread_mutex_unlock(&rw->mutex)) != 0)
        return err;

    return 0;
}

int rwlock_after_write(ReadWriteLock *rw) {
    int err;
    if ((err = pthread_mutex_lock(&rw->mutex)) != 0)
        return err;

    rw->n_writing--;

    if (rw->n_readers_waiting) {
        rw->change = 1;
        pthread_cond_broadcast(&rw->readers);
    } else {
        pthread_cond_signal((&rw->writers));
    }

    if ((err = pthread_mutex_unlock(&rw->mutex)) != 0)
        return err;

    return 0;
}

int rwlock_before_read(ReadWriteLock *rw) {
    int err;
    if ((err = pthread_mutex_lock(&rw->mutex)) != 0)
        return err;

    rw->n_readers_waiting++;

    while (rw->n_writing > 0 || rw->n_writers_waiting > 0) {
        if ((err = pthread_cond_wait(&rw->readers, &rw->mutex)) != 0)
            return err;
        if (rw->change == 1) {
            rw->change = 0;
            break;
        }
    }

    rw->n_readers_waiting--;
    rw->n_reading++;

    if ((err = pthread_mutex_unlock(&rw->mutex)) != 0)
        return err;

    return 0;
}

int rwlock_after_read(ReadWriteLock *rw) {
    int err;
    if ((err = pthread_mutex_lock(&rw->mutex)) != 0)
        return err;

    rw->n_reading--;

    if (rw->n_reading == 0)
        pthread_cond_signal(&rw->writers);

    if ((err = pthread_mutex_unlock(&rw->mutex)) != 0)
        return err;

    return 0;
}
