#ifndef WORKER_H
#define WORKER_H

#include <stdatomic.h>
#include "memory\load.h"

typedef enum
{
    WORKER_IDLE,
    WORKER_RUNNING,
    WORKER_DONE,
    WORKER_CANCELLED,
    WORKER_FAILED
} WorkerState;

typedef struct WorkerContext
{
    Base_List *list;
    Target *target;

    Base *result_base;
    Ammo_t *result_ammo;

    atomic_int state;

    atomic_uint work_done;
    atomic_uint work_total;

    atomic_int phase;
    atomic_uint current_base_id;
    atomic_uint current_ammo_id;
} WorkerContext;

int worker_entry(void *arg);

#endif
