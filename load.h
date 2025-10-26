#ifndef LOAD_H
#define LOAD_H

#include "storage.h"
#include "error.h"

#include <stdio.h>

typedef enum {
    NORMAL,          // reading data
    LINE_COMMENT,    // inside //
    BLOCK_COMMENT,   // inside /* ... */
    RECORD           // collecting record text until ';'
} ParseState;


Base *load_bases(const char *filename, unsigned *out_count);
Target *load_targets(const char *filename, unsigned *out_count);
int peek(FILE *file);

typedef HRESULT (*RecordHandler)(const char *record, void *data, int *error_flags);
HRESULT scan(const char *filename, RecordHandler handler, void *list);
HRESULT format_def_error(char *error_buffer, int *error_flags);

#endif