#ifndef LOAD_H
#define LOAD_H

#include "storage.h"
#include "error.h"
#include "calculations.h"

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

typedef HRESULT (*RecordHandler)(char *record, void *data, int *error_flags);
HRESULT scan(const char *filename, RecordHandler handler, void *list, PROCESS mode);
HRESULT format_basic_error_string(char *error_buffer, int *error_flags);

HRESULT generate_config(FILE *configfile, const char *config_file);
HRESULT recreate_config(const char *config_file, PROCESS mode);
HRESULT read_config(const char *config_file);
HRESULT handle_config(char *record, void *data, int *error_flags);

HRESULT test_and_run_config(const char *config_file, PROCESS mode);

HRESULT handle_ammo_t(char *record, void *data, int *error_flags);
HRESULT handle_base(char *record, void *data, int *error_flags);
HRESULT handle_target(char *record, void *data, int *error_flags);
// create a helper file for them or smth. idk

int HandleParameterSelection(void);
void LogResult(const Base *base, const Target *tgt, const Ammo_t *ammo, const BallisticResult *res);

#endif