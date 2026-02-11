/**
 * @file helpers.h
 * @brief Utility functions for string manipulation, path handling, and terminal formatting
 */
#ifndef HELPERS_H
#define HELPERS_H

#define HIDE_CURSOR() printf("\x1b[?25l")
#define SHOW_CURSOR() printf("\x1b[?25h")
#define NORMAL_SEPERATOR_SIZE 60
#define SIZE_OF_STRING_ARRAY(array) (sizeof(array) / sizeof(char *))
#define SIZE_OF_CHAR_ARRAY(array) (sizeof(array) / sizeof(char))

#include <ctype.h>
#include "helpers\debug.h"
#include "memory\storage.h"

/*Ensures string ends with backslash for directory paths */

char *ensure_trailing_backslash(char *str);

/*Normalizes path separators and removes duplicates */

char *Normalize_path(char *path);

/*Custom tokenizer with space/tab/newline/colon delimiters */

char *strtoken(char *s, char **endptr);

/*Removes matching quote characters from string ends */

char *remove_quotes(char *str, char symbol);

const char *skip_spaces_dptr(const char *str);

char *skip_spaces(char *str);

/*Case-insensitive string comparison with exact length check */

P_HRESULT norm_strncmp(const char *record, const char *str2);

/*Converts string to lowercase (ASCII only) - WARNING: has bug in implementation */

void NormalizeInput(char *input);

void print_substring(const char* start, const char* end);

/*Prints formatted separator line with optional centered text */

void add_seperator_c(const char *sep_colour, const char *str_colour, const char *str, char symbol, unsigned sep_length);

void add_seperator_s(const char *sep_colour, const char *str_colour, const char *str, const char *symbol, unsigned sep_length);

void add_header(size_t width, const char *box_colour, const char *format, ...);

/*Clears terminal using ANSI codes if color mode enabled */

void clear_terminal();

unsigned progress_percent(unsigned current, unsigned total);

typedef void (*progress_hook_fn)(void *arg);
void render_progress_bar(unsigned percent, progress_hook_fn before, progress_hook_fn after, void *arg_b, void *arg_a);

int clamp_with_warning(int value, int min, int max);

int clamp(int value, int min, int max);

void split_screen_demo(void);

void append_strings(char *buffer, const char *format, ...);

P_HRESULT replace_token_a(char *buffer, const char *token_id, const char *token, int (*validate_fp)(const char *));

char *replace_chars(char *source, const char replace_c, const char to_c);

int validate_int(const char* token);

int value_wrap_around(int state, int min, int max);

int validate_coordinate(Coordinate cord);

void reset_coordinate(Coordinate *cord);

char *get_token_from_buffer(const char *buffer, const char *token_id);

#endif