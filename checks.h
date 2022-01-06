#pragma once

#include <stdio.h>

// Sprawdza, czy funkcja systemowa się powiodła. W przypadku błędu kończy program z kodem 1.
#define CHECK_SYSERR(x)                                                           \
    do {                                                                          \
        int err = (x);                                                            \
        if (err != 0) {                                                           \
            fprintf(stderr, "Runtime error: %s returned %d in %s at %s:%d\n%s\n", \
                #x, err, __func__, __FILE__, __LINE__, strerror(err));            \
            exit(1);                                                              \
        }                                                                         \
    } while (0)

// Sprawdza, czy alokacja się powiodła. W przypadku błędu kończy program z kodem 1.
#define CHECK_PTR(x)                                                                    \
do {                                                                                    \
        void *err = (x);                                                                \
        if (err == NULL) {                                                              \
            fprintf(stderr, "Allocation error: %s returned NULL in %s at %s:%d\n",      \
                #x, __func__, __FILE__, __LINE__);                                      \
            exit(1);                                                                    \
        }                                                                               \
} while (0)
