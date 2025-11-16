#ifndef LOAD_H
#define LOAD_H

#include "storage.h"
#include "error.h"

#include <stdio.h>

typedef enum
{
    NORMAL,        // reading data
    LINE_COMMENT,  // inside //
    BLOCK_COMMENT, // inside /* ... */
    RECORD         // collecting record text until ';'
} ParseState;

extern char *folder_path;

void set_folder_path(const char *new_path);
void free_folder_path(void);

Base *load_bases(const char *filename, unsigned *out_count);
Target *load_targets(const char *filename, unsigned *out_count);
int peek(FILE *file);

typedef HRESULT (*RecordHandler)(const char *record, void *data, int *error_flags);
HRESULT scan(const char *filename, RecordHandler handler, void *list, PROCESS mode);
HRESULT format_basic_error_string(char *error_buffer, int *error_flags);

HRESULT generate_config(FILE *configfile, const char *config_file);
HRESULT read_config(const char *config_file);
HRESULT handle_config(const char *record, void *data, int *error_flags);

HRESULT test_and_run_config(const char *config_file, PROCESS mode);

//create a helper file for them or smth. idk
void ensure_trailing_backslash(char *str, size_t size);
void Normalize_path(char *path, size_t size);
const char* strtoken(const char *s, const char **endptr);
char *remove_quotes(char *str, char symbol);


#endif