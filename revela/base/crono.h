/* crono.h - medição monotônica de tempo de parede, em milissegundos.
 *
 * CLOCK_MONOTONIC e não CLOCK_REALTIME: um ajuste de relógio no meio da medição
 * não pode produzir tempo negativo.
 */
#ifndef REVELA_CRONO_H
#define REVELA_CRONO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

static inline double agora_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec * 1e3 + (double)t.tv_nsec / 1e6;
}

#ifdef __cplusplus
}
#endif

#endif
