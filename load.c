#include "load.h"
#include "calculations.h"
#include "error.h"
#include "commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// rewrite the loading to scan per charcter till ;, store the input, and format it, and search for identifing charcters I, N, C, A (area)
// add comment support, for easier readability

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

HRESULT scan(const char *filename, RecordHandler handler, void *data, PROCESS mode)
{
    FILE *file = fopen(filename, "r"); // "r" open file in read only mode
    if (!file)
    {
        printf("Failed to open or locate [%s] in the working directory!\n", filename);
        return S_FALSE;
    }

    int c;
    ParseState state;
    char buffer[256];
    int index = 0;
    HRESULT hr = S_OK; // handle to a result

    while ((c = fgetc(file)) != EOF && SUCCEEDED(hr))
    {
        switch (state)
        {
        case NORMAL:
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

        case LINE_COMMENT:
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

        case BLOCK_COMMENT:
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

        case RECORD:
            if (c == ';')
            {
                // terminate buffer
                buffer[index] = '\0';
                int error_flags[5] = {0};
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

HRESULT run_test_config(const char *config_file) // check if it's the first time, that the program is being ran, or if there is any missing info
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
                strcpy(temp, "colour");
                break;
            case CMD_MONO:
                strcpy(temp, "monochrome");
                break;
            }

            fprintf(configfile, "=====================================================\n");
            fprintf(configfile, "--------Parameters-------------------------------\n");
            fprintf(configfile, "working directory []\n");
            fprintf(configfile, "color mode []\n");
            fprintf(configfile, "=====================================================\n");
            fprintf(configfile, "-the id gets normalized\n");
            fprintf(configfile, "-end every line with ;\n");
            fprintf(configfile, "=====================================================\n");
            fprintf(configfile, "\n");
            fprintf(configfile, "DIR [%s];\n", folder_path);
            fprintf(configfile, "COLOURM [%s];\n", temp);

            fclose(configfile);
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

    configfile = fopen(config_file, "r");
    if (!configfile)
    {
        printf("Failed to open or locate [%s] in the working directory!\n", config_file);
        return S_FALSE;
    }

    if (fclose(configfile) == EOF)
        hr = E_FAIL;
    return hr;
}

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
