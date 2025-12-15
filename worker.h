#ifndef WORKER_H
#define WORKER_H

#include <stdatomic.h>
#include "load.h"

typedef enum {
    WORKER_IDLE,
    WORKER_RUNNING,
    WORKER_DONE,
    WORKER_CANCELLED,
    WORKER_FAILED
} WorkerState;

typedef struct WorkerContext {
    Base_List *list;
    Target *target;
    Base *result_base;
    Ammo_t *result_ammo;
    atomic_int state;
} WorkerContext;

int worker_entry(void *arg);

#endif
