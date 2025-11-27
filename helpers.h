#ifndef HELPERS_H
#define HELPERS_H

#include <ctype.h>

void ensure_trailing_backslash(char *str, size_t size);
void Normalize_path(char *path, size_t size);
const char* strtoken(const char *s, const char **endptr);
char *remove_quotes(char *str, char symbol);

#endif