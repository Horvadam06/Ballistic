#include "load.h"
#include "calculations.h"
#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// rewrite the loading to scan per charcter till ;, store the input, and format it, and search for identifing charcters I, N, C, A (area)
// add comment support, for easier readability

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
                          &temp.id, &temp.label, &temp.position.latitude, &temp.position.longitude, &temp.position.altitude);
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
                          &temp.id, &temp.label,
                          &temp.position.latitude, &temp.position.longitude, &temp.position.altitude,
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

HRESULT scan(const char *filename, RecordHandler handler, void *data)
{
    FILE *file = fopen(filename, "r"); // "r" open file in read only mode
    if (!file)
    {
        printf("Failed to open [%s]!\n", filename);
        return S_FALSE;
    }

    int c;
    ParseState state;
    char buffer[256];
    int index = 0;
    HRESULT hr = S_OK; // handle to a result

    while ((c = fgetc(file)) != EOF && hr == S_OK)
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
            printf("[Line comment]: ");
            while ((c = fgetc(file)) != EOF)
            {
                putchar(c);
                if (c == '\n')
                    break;
            }
            state = NORMAL;
            break;

        case BLOCK_COMMENT:
            printf("[Block comment]: ");
            while ((c = fgetc(file)) != EOF)
                if (c == '*' && peek(file) == '/')
                    break;
                else
                    putchar(c);

            fgetc(file);
            state = NORMAL;
            break;

        case RECORD:
            if (c == ';')
            {
                // terminate buffer
                buffer[index] = '\0';
                if (handler && data)
                    hr = handler(buffer, data); // pass the buffer and the user input address to the function
                else
                {
                    printf("No input handler!\n");
                    hr = S_FALSE;
                }
                index = 0;
                state = NORMAL;
            }
            else if (index < sizeof(buffer) - 1)
                buffer[index++] = (char)c;
            else {
                printf("[Error] buffer overflow!'");
                hr = S_FALSE;
            }

            break;
        }
    }

    if (fclose(file) != EOF)
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
