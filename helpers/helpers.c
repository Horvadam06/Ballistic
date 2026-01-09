#include "helpers.h"
#include "helpers\color.h"
#include "os\terminal_info.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
 * @return P_HRESULT P_S_OK if strings match exactly, S_FALSE otherwise
 *
 * @note Comparison is case-insensitive (A == a)
 * @note Returns S_FALSE if str2 is longer than record (not just a prefix match)
 * @note Example: "hello" vs "hell" returns S_FALSE
 */
P_HRESULT norm_strncmp(const char *record, const char *str2)
{
    int length = strlen(str2);
    for (int i = 0; record[i] != '\0' && i < length; i++)
    {
        char str = record[i];
        char str1 = str2[i];
        NormalizeInput(&str);
        NormalizeInput(&str1);

        if (str != str1)
            return P_S_FALSE;

        // Check if we've reached end of 'record' but 'str2' continues
        if (length - i > 1 && record[i + 1] == '\0')
            return P_S_FALSE;
    }

    return P_S_OK;
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

void print_substring(const char *start, const char *end)
{
    size_t length = end - start;
    if (length > 0)
    {
        fwrite(start, 1, length, stdout);
    }
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
 * @example add_seperator("Menu", '-', NORMAL_SEPERATOR_SIZE) prints: "------------ Menu ------------"
 * @example add_seperator(NULL, '=', NORMAL_SEPERATOR_SIZE) prints: "========================================"
 */
void add_seperator_c(const char *sep_colour, const char *str_colour, const char *str, char symbol, unsigned sep_length)
{
    size_t str_length = (str) ? strlen(str) : 0;
    unsigned start = (sep_length - str_length) / 2; // Center position
    unsigned cnt = 0;
    printf(ANSI_RESET);

    // Print leading symbols
    if (sep_colour)
        printf(sep_colour);
    for (cnt = 0; cnt < start; cnt++)
        printf("%c", symbol);

    // Print centered text (if any)
    if (str_colour)
        printf(str_colour);
    if (str)
        printf(str);

    printf(ANSI_RESET);

    // Print trailing symbols to fill remaining length
    if (sep_colour)
        printf(sep_colour);
    for (cnt += str_length; cnt < sep_length; cnt++)
        printf("%c", symbol);

    printf(ANSI_RESET "\n");
}

void add_seperator_s(const char *sep_colour, const char *str_colour, const char *str, const char *symbol, unsigned sep_length)
{
    size_t str_length = (str) ? strlen(str) : 0;
    unsigned start = (sep_length - str_length) / 2; // Center position
    unsigned cnt = 0;
    printf(ANSI_RESET);

    // Print leading symbols
    if (sep_colour)
        printf(sep_colour);
    for (cnt = 0; cnt < start; cnt++)
        printf("%s", symbol);

    // Print centered text (if any)
    if (str_colour)
        printf(str_colour);
    if (str)
        printf(str);

    printf(ANSI_RESET);

    // Print trailing symbols to fill remaining length
    if (sep_colour)
        printf(sep_colour);
    for (cnt += str_length; cnt < sep_length; cnt++)
        printf("%s", symbol);

    printf(ANSI_RESET "\n");
}

#include <stdarg.h>

// if 0 is the width, auto clamp to the input length, use │ for seperation
void print_header(size_t width, const char *box_colour, const char *format, ...)
{
    size_t row_n = 3;
    if (width == 0)
        return;
    va_list ap;

    va_start(ap, format);

    // modify later to use dynamic memory alloc
    char buffer[1024];
    vsprintf(buffer, format, ap);

    // or the phrased length
    for (size_t row = 1; row <= row_n; row++)
    {
        printf("%s", box_colour);
        printf("%s", (row == 1) ? "┌" : ((row == row_n) ? "└" : "│"));
        printf(ANSI_RESET);
        // can do it because of the end brackets colum < width comes from colum <= width - 1
        int j = 0;
        for (size_t column = 2; column < width; column++)
        {
            if (row == 1 || row == row_n)
            {
                printf("%s", box_colour);
                if (j == 0)
                {
                    int in_esc_code = 0;
                    int trimmed_length = 0;
                    for (; buffer[j] != '\0'; j++)
                    {
                        if (buffer[j] == '\033')
                            in_esc_code = 1;
                        else if (in_esc_code && buffer[j] == 'm')
                            in_esc_code = 0;
                        else if (!in_esc_code)
                        {
                            if (buffer[j] == '|')
                            {
                                printf((row == 1) ? "┬" : "┴");
                            }
                            else
                                printf("─");
                            trimmed_length++;
                        }
                    }
                    column = trimmed_length + 1;
                }

                printf("─");
                /*if (j == 0)
                {
                    int esc_code_len_sum = 0;
                    for (; buffer[j] != '\0'; j++)
                        if (buffer[j] == '\033')
                        {
                            for (; buffer[j] != '\0' && buffer[j] != 'm'; j++, esc_code_len_sum++)
                                ;
                        }
                        else if (buffer[j] == '|')
                            printf("│");
                        else

                            column = j + 1 - esc_code_len_sum;
                }
                */
            }
            else
            {

                if (j == 0)
                {
                    int in_esc_code = 0;
                    int trimmed_length = 0;
                    for (; buffer[j] != '\0'; j++)
                    {
                        if (buffer[j] == '\033')
                            in_esc_code = 1;
                        else if (in_esc_code && buffer[j] == 'm')
                            in_esc_code = 0;
                        else if (!in_esc_code)
                            trimmed_length++;

                        if (buffer[j] == '|')
                            printf("│");
                        else
                            printf("%c", buffer[j]);
                    }
                    column = trimmed_length + 1;
                }

                putchar(' ');
            }
        }
        printf("%s", box_colour);
        printf("%s\n", (row == 1) ? "┐" : ((row == row_n) ? "┘" : "│"));
        printf(ANSI_RESET);
    }
    va_end(ap);
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

unsigned progress_percent(unsigned current, unsigned total)
{
    if (total == 0)
        return 100;

    if (current >= total)
        return 100;

    return (current * 100u) / total;
}

void render_progress_bar(unsigned percent, progress_hook_fn before, progress_hook_fn after, void *arg_b, void *arg_a)
{
    const unsigned bar_width = 30;

    if (percent > 100)
        percent = 100;

    unsigned filled = (percent * bar_width) / 100;
    if (before)
    {
        printf("\r");
        before(arg_b);
        printf("[");
    }
    else
        printf("\r[");

    for (unsigned i = 0; i < bar_width; ++i)
        printf(i < filled ? (COLOR_RESET">") : (COLOR_HINT " "));

    printf(COLOR_RESET"] %3u%%", percent);
    if (after)
        after(arg_a);
    fflush(stdout);
}

const char *skip_spaces(const char *str)
{
    if (!str)
        return NULL;

    while (*str == ' ')
        str++;

    return str;
}


void split_screen_demo(void) {
    int width, height;
    terminal_size(&width, &height);
    
    // Clear screen
    clear_terminal();
    
    // Draw vertical line in middle
    for (int y = 0; y < height - 2; y++) {
        printf("\033[%d;%dH│", y + 1, width / 2);
    }
    
    // Write to left panel
    printf("\033[2;2H");  // Move to row 2, column 2
    printf("LEFT PANEL");
    printf("\033[3;2H");
    printf("Item 1");
    printf("\033[4;2H");
    printf("Item 2");
    
    // Write to right panel
    printf("\033[2;%dH", width / 2 + 2);
    printf("RIGHT PANEL");
    printf("\033[3;%dH", width / 2 + 2);
    printf("Value A");
    printf("\033[4;%dH", width / 2 + 2);
    printf("Value B");
    
    // Keep cursor at bottom
    printf("\033[%d;1H", height);
}

