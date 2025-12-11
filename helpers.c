#include "helpers.h"
#include "color.h"

#include <string.h>
#include <stdlib.h>

#include "debugmalloc.h"

/**
 * @brief Ensures a string ends with a backslash
 *
 * Appends a backslash to the end of a string if it doesn't already end with one.
 * Useful for constructing directory paths where trailing separator is required.
 *
 * @param str Null-terminated string (must have space for at least one extra character)
 * @return Pointer to the modified string
 *
 * @warning Assumes the buffer has at least strlen(str) + 2 bytes available
 * @warning Uses Windows-style backslashes - not portable to Unix systems
 * @note Modifies the input string in-place
 */
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

/**
 * @brief Normalizes path separators in a file path
 *
 * Converts all forward slashes and backslashes to forward slashes,
 * and collapses multiple consecutive separators into a single one.
 *
 * @param path File path string to normalize
 * @return Pointer to the normalized string (same as input)
 *
 * @note Converts to forward slashes (Unix-style) regardless of platform
 * @note Collapses "//" or "\\" or "/\" etc. into single "/"
 * @note Works in-place without allocating additional memory
 */
char *Normalize_path(char *path)
{
    if (!path)
        return NULL;

    int offset = 0; // Tracks how many characters we've removed due to duplicate separators

    for (int i = 0; path[i] != '\0'; i++)
    {
        // Skip duplicate separators
        for (int j = 0; (path[i] == '/' || path[i] == '\\'); i++)
        {
            if (j == 0) // Keep the first separator
            {
                j++;
                path[i + offset] = '/';
                continue;
            }

            offset--; // Skip this duplicate separator
        }

        path[i + offset] = path[i];
    }

    return path;
}


/**
 * @brief Custom string tokenizer
 * 
 * Extracts tokens separated by space, tab, newline, or colon characters.
 * Similar to strtok but without static state.
 * 
 * @param str String to tokenize (will be modified if delimiters are replaced)
 * @param endptr Pointer to store the position after the token (can be NULL)
 * @return Pointer to the beginning of the token
 * 
 * @note Delimiters are: space, tab, newline, colon
 */
char *strtoken(char *str, char **endptr)
{
    // Skip leading delimiters
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == ':')
        str++;

    char *start = str;

    // Find end of token
    while (*str && *str != ' ' && *str != '\t' && *str != '\n' && *str != ':')
        str++;

    if (endptr)
        *endptr = str;

    return start;
}

/**
 * @brief Removes matching quote characters from the beginning and end of a string
 * 
 * Removes the specified quote character if it appears at both ends of the string.
 * Useful for processing quoted strings from configuration files or user input.
 * 
 * @param str String to unquote
 * @param symbol Quote character to remove (typically '"' or '\'')
 * @return Pointer to the unquoted string (may be offset if opening quote removed)
 * 
 * @note Only removes quotes if they appear at BOTH ends
 * @note If string starts with quote but doesn't end with one, only opening quote is removed
 * @note Modifies the original string in-place
 */
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

/**
 * @brief Normalized string comparison with length checking
 * 
 * Compares two strings case-insensitively and ensures they match completely
 * (not just as a prefix). Both strings are normalized to lowercase during comparison.
 * 
 * @param record First string to compare
 * @param str2 Second string to compare
 * @return HRESULT S_OK if strings match exactly, S_FALSE otherwise
 * 
 * @note Comparison is case-insensitive (A == a)
 * @note Returns S_FALSE if str2 is longer than record (not just a prefix match)
 * @note Example: "hello" vs "hell" returns S_FALSE
 */
HRESULT norm_strncmp(const char *record, const char *str2)
{
    int length = strlen(str2);
    for (int i = 0; record[i] != '\0' && i < length; i++)
    {
        char str = record[i];
        char str1 = str2[i];
        NormalizeInput(&str);
        NormalizeInput(&str1);

        if (str != str1)
            return S_FALSE;

        // Check if we've reached end of 'record' but 'str2' continues
        if (length - i > 1 && record[i + 1] == '\0')
            return S_FALSE;
    }

    return S_OK;
}

/**
 * @brief Converts a string to lowercase in-place
 * 
 * Modifies ASCII characters 'A'-'Z' to their lowercase equivalents.
 * Leaves other characters unchanged.
 * 
 * @param input String to normalize (modified in-place)
 * 
 * @bug Uses sizeof(input) instead of strlen(input) - will only process
 *      first sizeof(pointer) bytes (typically 4 or 8)
 * @bug Should be: for (unsigned i = 0; input[i] != '\0'; i++)
 * @note Only handles ASCII characters, not extended Unicode
 */
void NormalizeInput(char *input)
{
    for (unsigned i = 0; i < strlen(input); i++)
        if (input[i] >= 'A' && input[i] <= 'Z')
            input[i] += 'a' - 'A';
}

/**
 * @brief Prints a formatted separator line with optional centered text
 * 
 * Creates a visual separator using repeated characters, with
 * centered text. Useful for creating menu borders and section dividers.
 * 
 * @param str Text to center within the separator (NULL for no text)
 * @param symbol Character to repeat for the separator line
 * @param sep_length Total length of the separator line
 * 
 * @example add_seperator("Menu", '-', 30) prints: "------------ Menu ------------"
 * @example add_seperator(NULL, '=', 40) prints: "========================================"
 */
void add_seperator(const char *str, char symbol, unsigned sep_length)
{
    size_t str_length = (str) ? strlen(str) : 0;
    unsigned start = (sep_length - str_length) / 2;  // Center position
    unsigned cnt = 0;

    // Print leading symbols
    for (cnt = 0; cnt < start; cnt++)
    {
        printf("%c", symbol);
    };
    
    // Print centered text (if any)
    if (str)
        printf("%s", str);
    
    // Print trailing symbols to fill remaining length
    for (cnt += str_length; cnt < sep_length; cnt++)
    {
        printf("%c", symbol);
    };
    
    printf("\n");
}

/**
 * @brief Clears the terminal screen using ANSI escape codes
 * 
 * Only works when color mode is enabled (CM_COLOUR). In monochrome mode,
 * this function does nothing. Uses standard ANSI escape sequences.
 * 
 * @note \033[2J clears the entire screen
 * @note \033[H moves cursor to home position (1,1)
 * @note Will not work on terminals without ANSI support
 */
void clear_terminal()
{
    if (get_color_mode() == CM_COLOUR)
        printf("\033[2J\033[H");
};
