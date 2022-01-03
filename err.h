#pragma once

/* wypisuje informacje o błędnym zakończeniu funkcji systemowej
i kończy działanie */
extern void syserr(const char* fmt, ...);

/* wypisuje informacje o błędzie i kończy działanie */
extern void fatal(const char* fmt, ...);

// Sprawdza, czy alokacja się powiodła. W przypadku błędu kończy program z kodem 1.
#define CHECK_PTR(p)                    \
    do {                                \
        if (p == NULL) {                \
            fatal("malloc() failed");   \
        }                               \
    } while (0)
