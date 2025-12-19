/**
 * @file helpers.h
 * @brief Utility functions for string manipulation, path handling, and terminal formatting
 */
#ifndef HELPERS_H
#define HELPERS_H

#define HIDE_CURSOR() printf("\x1b[?25l")
#define SHOW_CURSOR() printf("\x1b[?25h")
#define NORMAL_SEPERATOR_SIZE 60

#include <ctype.h>
#include "helpers\debug.h"

/*Ensures string ends with backslash for directory paths */

char *ensure_trailing_backslash(char *str);

/*Normalizes path separators and removes duplicates */

char *Normalize_path(char *path);

/*Custom tokenizer with space/tab/newline/colon delimiters */

char *strtoken(char *s, char **endptr);

/*Removes matching quote characters from string ends */

char *remove_quotes(char *str, char symbol);

const char *skip_spaces(const char *str);

/*Case-insensitive string comparison with exact length check */

P_HRESULT norm_strncmp(const char *record, const char *str2);

/*Converts string to lowercase (ASCII only) - WARNING: has bug in implementation */

void NormalizeInput(char *input);

/*Prints formatted separator line with optional centered text */

void add_seperator_c(const char *sep_colour, const char *str_colour, const char *str, char symbol, unsigned sep_length);

void add_seperator_s(const char *sep_colour, const char *str_colour, const char *str, const char *symbol, unsigned sep_length);

/*Clears terminal using ANSI codes if color mode enabled */

void clear_terminal();

unsigned progress_percent(unsigned current, unsigned total);

typedef void (*progress_hook_fn)(void *arg);
void render_progress_bar(unsigned percent, progress_hook_fn before, progress_hook_fn after, void *arg_b, void *arg_a);

#endif