#include "debug.h"
#include <stdio.h>

void debug_echo (const char* command, int length) {
    for (int i = 0; i<length; i++) printf("%c", command[i]);
    printf("\n");
}

DebugNamespace Debug = {
    .echo = debug_echo,
//    .log  = debug_log
};