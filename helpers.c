#include "helpers.h"

#include <string.h>
#include <stdlib.h>

void ensure_trailing_backslash(char *str, size_t size)
{
    size_t len = strlen(str);

    // Check bounds and avoid overflow
    if (len + 1 >= size)
        return;

    if (len == 0 || str[len - 1] != '\\')
    {
        str[len] = '\\';     // place backslash before null terminator
        str[len + 1] = '\0'; // new null terminator
    }
}

void Normalize_path(char *path, size_t size)
{
    if (!path || size == 0)
        return;

    char *buffer = malloc(size);
    if (!buffer)
        return;

    size_t j = 0;
    for (size_t i = 0; path[i] != '\0' && j + 2 < size; i++)
    {
        if (path[i] == '/')
            buffer[j++] = '\\';
        else if (path[i] == (char)92)
        {
            if (path[i + 1] != (char)92)
                buffer[j++] = '\\';
            /*if (path[i + 1] != '\\' && j + 1 < size)
                buffer[j++] = '\\';*/
        }
        else
            buffer[j++] = path[i];
    }

    buffer[j] = '\0';
    strncpy(path, buffer, size - 1);
    path[size - 1] = '\0';

    free(buffer);
}

const char *strtoken(const char *str, const char **endptr)
{
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == ':')
        str++;

    const char *start = str;

    while (*str && *str != ' ' && *str != '\t' && *str != '\n' && *str != ':')
        str++;

    if (endptr)
        *endptr = str;

    return start;
}

char *remove_quotes(char *str, char symbol)
{
    int len = strlen(str);
    if (len < 2)
        return str;

    char first = str[0];
    char last = str[len - 1];

    if (last == symbol)
        str[len - 1] = '\0'; // remove closing quote
    if (first == symbol)
        return str + 1; // skip the opening quote

    return str;
}
