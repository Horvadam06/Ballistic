#ifndef HELPERS_H
#define HELPERS_H

#include <ctype.h>
#include "debug.h"

char *ensure_trailing_backslash(char *str);
char *Normalize_path(char *path);
char *strtoken(char *s, char **endptr);
char *remove_quotes(char *str, char symbol);
HRESULT norm_strncmp(const char *record, const char *str2);
void NormalizeInput(char *input);

#endif