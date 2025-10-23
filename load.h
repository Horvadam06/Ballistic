#ifndef LOAD_H
#define LOAD_H

#include "calculations.h"

#include <stdio.h>

typedef enum {
    NORMAL,          // reading data
    LINE_COMMENT,    // inside //
    BLOCK_COMMENT,   // inside /* ... */
    RECORD           // collecting record text until ';'
} ParseState;


base *load_bases(const char *filename, unsigned *out_count);
target *load_targets(const char *filename, unsigned *out_count);
int peek(FILE *file);
int scan(const char *filename);

#endif