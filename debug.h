#ifndef DEBUG_H
#define DEBUG_H

typedef struct {
    void (*echo)(const char* command, int length);
    //void (*log)(const char *msg);
} DebugNamespace;

extern DebugNamespace Debug;

#endif