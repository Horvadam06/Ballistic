#ifndef DEBUG_H
#define DEBUG_H

#include "error.h"

#define EMPTY_MESSAGE printf("Nothing to see here! (yet)\n");

typedef enum
{
    false,
    true
} bool;

P_HRESULT run_pre_test(PROCESS process_state);

#endif