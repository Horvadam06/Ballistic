#include "load.h"
#include "calculations.h"
#include "error.h"
#include "commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <color.h>

// seperate config from loading, beacuse it's messy asf

char *folder_path = NULL;

void set_folder_path(const char *new_path)
{
    if (!new_path)
        return;

    // Free old memory if already allocated
    free(folder_path);

    // Allocate new memory for the new path
    folder_path = malloc(strlen(new_path) + 1);
    if (!folder_path)
    {
        fprintf(stderr, "Memory allocation failed for folder_path\n");
        return;
    }

    strcpy(folder_path, new_path);
}

void free_folder_path(void)
{
    free(folder_path);
    folder_path = NULL;
}

Base *load_bases(const char *filename, unsigned *out_count)
{
    FILE *file = fopen(filename, "r"); // "r" open file in read only mode
    if (!file)
    {
        perror("Failed to open file");
        return NULL;
    }

    unsigned capacity = 5;
    unsigned count = 0;
    Base *bases = malloc(capacity * sizeof(Base));
    if (!bases)
    {
        fclose(file);
        return NULL;
    }

    while (1)
    {
        Base temp;
        int read = fscanf(file, "%d %63s %lf %lf %lf", // limits the input string length to 63 characters
                          &temp.info.id, &temp.info.label, &temp.info.position.latitude, &temp.info.position.longitude, &temp.info.position.altitude);
        if (read == 5)
        {
            if (count >= capacity)
            {
                capacity *= 2;
                Base *resized = realloc(bases, capacity * sizeof(Base));
                if (!resized)
                {
                    perror("Memory allocation failed");
                    free(bases);
                    fclose(file);
                    return NULL;
                }
                bases = resized;
            }
            bases[count++] = temp;
        }
        else if (read == EOF || read == 0)
            break; // EOF or invalid line
        else
        {
            // Skip malformed line
            char buffer[256];
            fgets(buffer, sizeof(buffer), file);
        }
    }

    *out_count = count;
    fclose(file);

    return bases;
}

Target *load_targets(const char *filename, unsigned *out_count)
{
    FILE *file = fopen(filename, "r"); // "r" open file in read only mode
    if (!file)
    {
        perror("Failed to open file");
        return NULL;
    }

    unsigned capacity = 5;
    unsigned count = 0;
    Target *targets = malloc(capacity * sizeof(Target));
    if (!targets)
    {
        fclose(file);
        return NULL;
    }

    while (1)
    {
        Target temp;
        int read = fscanf(file, "%d %63s %lf %lf %lf %lf", // limits the input string length to 63 characters
                          &temp.info.id, &temp.info.label,
                          &temp.info.position.latitude, &temp.info.position.longitude, &temp.info.position.altitude,
                          &temp.area);
        if (read == 6)
        {
            if (count >= capacity)
            {
                capacity *= 2;
                Target *resized = realloc(targets, capacity * sizeof(Target));
                if (!resized)
                {
                    perror("Memory allocation failed");
                    free(targets);
                    fclose(file);
                    return NULL;
                }
                targets = resized;
            }
            targets[count++] = temp;
        }
        else if (read == EOF || read == 0)
            break; // EOF or invalid line
        else
        {
            // Skip malformed line
            char buffer[256];
            fgets(buffer, sizeof(buffer), file);
        }
    }

    *out_count = count;
    fclose(file);

    return targets;
}

/*file name to scan  |  handler function  |  external data  |  scan mode*/
HRESULT scan(const char *filename, RecordHandler handler, void *data, PROCESS mode)
{
    FILE *file = fopen(filename, "r"); // "r" open file in read only mode
    if (!file)
    {
        printf("Failed to open or locate [%s] in the working directory!\n", filename);
        return S_FALSE;
    }

    int c;
    ParseState state = NORMAL;
    char buffer[256];
    int index = 0;
    HRESULT hr = S_OK; // handle to a result

    while ((c = fgetc(file)) != EOF && SUCCEEDED(hr))
    {
        switch (state)
        {
        case NORMAL:
        {
            if (c == '/')
            {
                int value = peek(file);
                if (value == '/')
                    state = LINE_COMMENT;
                else if (value == '*')
                    state = BLOCK_COMMENT;
                else
                { // invalid line
                    fprintf(stderr, "Warning: invalid syntax, skipping line\n");
                    while ((c = fgetc(file)) != EOF && c != '\n')
                        ;
                }
            }
            else if (!isspace(c))
            {
                // start buffering a record
                ungetc(c, file);
                state = RECORD;
            }

            break;
        }
        case LINE_COMMENT:
        {
            if (mode == S_DEBUG)
                printf("[Line comment] : \n");
            while ((c = fgetc(file)) != EOF)
            {
                if (mode == S_DEBUG)
                    putchar(c);
                if (c == '\n')
                    break;
            }
            state = NORMAL;
            break;
        }
        case BLOCK_COMMENT:
        {
            if (mode == S_DEBUG)
                printf("[Block comment] : \n");
            while ((c = fgetc(file)) != EOF)
                if (c == '*' && peek(file) == '/')
                    break;
                else if (mode == S_DEBUG)
                    putchar(c);

            fgetc(file);
            state = NORMAL;
            break;
        }
        case RECORD:
        {
            if (c == ';')
            {
                // terminate buffer
                buffer[index] = '\0';
                int error_flags[5] = {0};
                // a 256 sized array gets passed to the handler, so you can safely step it's ptr
                if (handler && data)
                    hr = handler(buffer, data, error_flags); // pass the buffer and the user input address to the function

                else
                {
                    printf("No input handler!\n");
                    hr = S_FALSE;
                }

                if (hr == S_PARTIAL)
                {
                    char error[128] = {0};
                    if (format_basic_error_string(error, error_flags) == S_FALSE)
                        printf("[Error] : Error buffer overflow!\n");
                    printf("[Warning] : Missing parameter in the file [%s] : \nat the line [%s] : \n suggested parameters to add [%s]\n", filename, buffer, error);
                }
                index = 0;
                state = NORMAL;
            }
            else if (index < sizeof(buffer) - 1) // update the buffer loader to not store useless spaces, only store 1
            {
                if (isspace(c))
                    state = NORMAL;
                buffer[index++] = (char)c;
            }
            else
            {
                printf("[Error] buffer overflow!");
                hr = S_FALSE;
            }

            break;
        }
        }
    }

    if (fclose(file) == EOF)
        hr = E_FAIL;
    return hr;
}

int peek(FILE *file)
{
    int c = fgetc(file);
    if (c != EOF)
        ungetc(c, file); // put it back
    return c;
}

HRESULT format_basic_error_string(char *error_buffer, int *error_flags)
{
    int er_cnt = -1;

    for (int i = 0; i < 5; i++)
    {
        if (error_flags[i] != 1)
        {
            er_cnt++;
            switch (i)
            {
            case F_ID:
            {
                const char *id = "I";
                if (er_cnt > 0)
                    id = ", I";
                strncat(error_buffer, id, sizeof(error_buffer) - strlen(error_buffer) - 1);
                continue;
            }
            case F_LABEL:
            {
                const char *id = "L";
                if (er_cnt > 0)
                    id = ", L";
                strncat(error_buffer, id, sizeof(error_buffer) - strlen(error_buffer) - 1);
                continue;
            }
            case F_LATITUDE:
            {
                const char *id = "CLA";
                if (er_cnt > 0)
                    id = ", CLA";
                strncat(error_buffer, id, sizeof(error_buffer) - strlen(error_buffer) - 1);
                continue;
            }
            case F_LONGITUDE:
            {
                const char *id = "CLO";
                if (er_cnt > 0)
                    id = ", CLO";
                strncat(error_buffer, id, sizeof(error_buffer) - strlen(error_buffer) - 1);
                continue;
            }
            case F_ALTITUDE:
            {
                const char *id = "CA";
                if (er_cnt > 0)
                    id = ", CA";
                strncat(error_buffer, id, sizeof(error_buffer) - strlen(error_buffer) - 1);
                continue;
            }
            }
            return S_FALSE;
        }
    }
    strncat(error_buffer, "\0", sizeof(error_buffer) - strlen(error_buffer));
    return S_OK;
}

// config===========================================

HRESULT test_and_run_config(const char *config_file, PROCESS mode) // check if it's the first time, that the program is being ran, or if there is any missing info
{
    HRESULT hr = S_OK;
    FILE *configfile = fopen(config_file, "r"); // "r" open file in read only mode
    if (!configfile)
    {
        printf("Failed to open or locate [%s] in the working directory!\n", config_file);
        printf("Do you want to create a new one?\n");

        switch (GetCommand(CMD_C_LOOP | CMD_C_YESNO, NULL))
        {
        case CMD_YES:
        {
            hr = generate_config(configfile, config_file);
            break;
        }
        case CMD_NO:
        {
            hr = E_FAIL;
            break;
        }
        }
        return hr;
    }

    hr = read_config(config_file);
    if (SUCCEEDED(hr))
    {
        if (mode == S_DEBUG)
            printf("Working directory have been set to: [%s]\n", folder_path);
        printf("----------------------------------\n");
        printf("Config loaded succesfully!\n");
        printf("----------------------------------\n");
        printf("\n");
    }
    return hr;
}

HRESULT generate_config(FILE *configfile, const char *config_file)
{
    char temp[1024]; // temporary buffer for user input
    printf("Enter a folder path:\n");
    /*
    after reading the length of the data, for example 10 bytes, it leaves the rest there,
    which cause the next command read to read in a false value
    if (scanf("%1023s", temp) == 1) // read at most 1023 chars, prevent overflow
        set_folder_path(temp);      // pass directly to your dynamic setter
    */
    if (fgets(temp, sizeof(temp), stdin))
    {
        /*char *pos = strchr(temp, '\n'); // find the first newline
        if (pos)                        // overwrite starting at '\n'
            strcpy(pos, config_file);   // used instead of cat, beacause I want to copy the string to a specific
                                        // location instead of at the end of a string
        else
            strcat(temp, config_file);*/

        temp[strcspn(temp, "\n")] = '\0'; // remove trailing newline

        ensure_trailing_backslash(temp, sizeof(temp));
        Normalize_path(temp, sizeof(temp));

        set_folder_path(temp);
    }

    configfile = fopen(config_file, "w"); // create a new config file
    if (!config_file)
        return S_FALSE;

    printf("Enter the colour mode:\n");

    switch (GetCommand(CMD_C_LOOP | CMD_C_SETUP, NULL))
    {
    case CMD_COLOUR:
        set_color_mode(CM_COLOUR);
        break;
    case CMD_MONO:
        set_color_mode(CM_MONO);
        break;
    }

    fprintf(configfile, "/*\n=====================================================\n");
    fprintf(configfile, "--------Parameters-------------------------------\n");
    fprintf(configfile, "working directory :\n");
    fprintf(configfile, "color mode : [0=monochrome, 1=coloured]\n");
    fprintf(configfile, "=====================================================\n");
    fprintf(configfile, "-the id gets normalized\n");
    fprintf(configfile, "-end every line with ;\n");
    fprintf(configfile, "=====================================================\n*/\n");
    fprintf(configfile, "\n");
    fprintf(configfile, "DIR: \"%s\";\n", folder_path);
    fprintf(configfile, "COLOUR_M: %d;\n", get_color_mode());

    if (fclose(configfile) == 0)
        return S_OK;
    printf("\n=========================\n");
    printf("[ERROR] Config file creation error");
    printf("\n=========================\n");
    return S_FALSE;
}

HRESULT read_config(const char *config_file)
{

    FILE *configfile = fopen(config_file, "r");
    if (!configfile)
    {
        printf("Failed to open or locate [%s] in the working directory!\n", config_file);
        return S_FALSE;
    }

    scan(config_file, handle_config, "idk", S_RUNNING);

    if (fclose(configfile) == EOF)
        return E_FAIL;

    return S_OK;
}

// rewrite this mess
HRESULT handle_config(const char *record, void *data, int *error_flags)
{

    /*InputFlags flag = T_WHITESPACE;
    HRESULT hr = S_OK;
    int length = strlen(record);
    int label_start = 0;*/

    const char *end = NULL;
    /*searches for the normlaized DIR token, then if it is, searches for the value after the ':' charcter*/
    if (norm_strncmp(strtoken(record, &end), "DIR") == S_OK && (record = strchr(end, ':')) != NULL)
    {
        set_folder_path(remove_quotes((char *)strtoken(record, &record), '\"'));
        return S_OK;
    }

    /*searches for the normlaized COLOUR_M token, then if it is, searches for the value after  the ':' charcter*/
    if (norm_strncmp(strtoken(record, &end), "COLOUR_M") == S_OK && (record = strchr(end, ':')) != NULL)
    {
        set_color_mode((int)strtof(strtoken(record, &end), (char **)record));
        return S_OK;
    }

    /*    for (int i = 0; record[i] != '\0' && i < length; i++)
        {
            // make all this shit tokenized
            char c = record[i];
            char next_c = (record[i + 1]) ? record[i + 1] : '\0';

                    switch (flag)
                    {
                    case T_WHITESPACE:
                    {
                        c = tolower(c);

                        switch (c)
                        {
                        case 'd':
                        case 'c':
                            flag = T_DATA;
                            break;
                        default:
                            const char *p = strchr(&record[i], ' ');
                            if (p)
                                i = p - record;
                            break;
                        }
                        break;
                    }
                    case T_DATA:
                    {
                        if (isspace(c) || c == '\n' || c == '\0')
                        {
                            flag = T_WHITESPACE;
                            break;
                        }
                        if (norm_strncmp(&record[i], "IR") == S_OK) // don't use the first letter of the word
                        {                                           // because it have been already checked
                            i += 2;
                            char *result = (char *)record;
                            if ((result = strchr(result, ':')) != NULL)
                            {
                                result++;
                                const char *end = NULL;
                                set_folder_path(remove_quotes((char *)strtoken(result, &end), '\"'));
                                i = end - record;

                                // get the string until the ']' shit, and use the standard process on it
                            }
                            // implement something to offset the i value or no, idk
                            // i = end - record;
                            flag = T_WHITESPACE;
                        }
                        else if (norm_strncmp(&record[i], "OLOUR_M") == S_OK)
                        {
                            const char *result = record;
                            if ((result = strchr(result, ':')) != NULL)
                            {
                                result++;
                                char *end = NULL;
                                set_color_mode((int)strtof(strtoken(result, (const char **)end), &end));
                                i = end - record;
                            }
                            // implement something to offset the i value or no, idk

                            flag = T_WHITESPACE;
                        }
                        else
                            flag = T_WHITESPACE;

                        break;
                    }
                    default:
                        return E_FAIL; // could only happen, if something goes very wrong
                    }
                }
            */
    return S_OK;
}

// create a sub file for these...things

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
