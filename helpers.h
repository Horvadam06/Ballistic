#ifndef HELPERS_H
#define HELPERS_H

#include <ctype.h>

char *ensure_trailing_backslash(char *str);
char *Normalize_path(char *path);
char* strtoken(char *s, char **endptr);
char *remove_quotes(char *str, char symbol);

#endif