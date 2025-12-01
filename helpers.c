#include "helpers.h"

#include <string.h>
#include <stdlib.h>

#include "debugmalloc.h"

char *ensure_trailing_backslash(char *str)
{
    size_t len = strlen(str);

    if (len == 0 || str[len - 1] != '\\')
    {
        str[len] = '\\';     // place backslash before null terminator
        str[len + 1] = '\0'; // new null terminator
    }
    return str;
}

char *Normalize_path(char *path)
{
    if (!path)
        return NULL;

    int offset = 0;

    for (int i = 0; path[i] != '\0'; i++)
    {

        for (int j = 0; (path[i] == '/' || path[i] == '\\'); i++)
        {
            if (j == 0)
            {
                j++;
                path[i + offset] = '/';
                continue;
            }

            offset--;
        }

        path[i + offset] = path[i];
    }

    return path;
}

//returns a token from a string, and changes the end ptr
char *strtoken(char *str, char **endptr)
{
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == ':')
        str++;

    char *start = str;

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


HRESULT norm_strncmp(const char *record, const char *str2)
{
    size_t length = strlen(str2);
    for (int i = 0; record[i] != '\0' && i < length; i++)
    {
        char str = record[i];
        char str1 = str2[i];
        NormalizeInput(&str);
        NormalizeInput(&str1);

        if (str != str1)
            return S_FALSE;

        if (length - i > 1 && record[i + 1] == '\0')
            return S_FALSE;
    }

    return S_OK;
}

void NormalizeInput(char *input)
{
    for (int i = 0; i < sizeof(input); i++)
        if (input[i] >= 'A' && input[i] <= 'Z')
            input[i] += 'a' - 'A';
}
