/**
 * @file helpers.h
 * @brief Utility functions for string manipulation, path handling, and terminal formatting
 */
#ifndef HELPERS_H
#define HELPERS_H

#include <ctype.h>
#include "debug.h"

/*Ensures string ends with backslash for directory paths */

char *ensure_trailing_backslash(char *str);

/*Normalizes path separators and removes duplicates */

char *Normalize_path(char *path);

/*Custom tokenizer with space/tab/newline/colon delimiters */

char *strtoken(char *s, char **endptr);

/*Removes matching quote characters from string ends */

char *remove_quotes(char *str, char symbol);

/*Case-insensitive string comparison with exact length check */

HRESULT norm_strncmp(const char *record, const char *str2);

/*Converts string to lowercase (ASCII only) - WARNING: has bug in implementation */

void NormalizeInput(char *input);

/*Prints formatted separator line with optional centered text */

void add_seperator(const char *str, char symbol, unsigned sep_length);

/*Clears terminal using ANSI codes if color mode enabled */

void clear_terminal();

#endif