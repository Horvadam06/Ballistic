#include "storage.h"
#include "commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void init_base_list(BaseList *list)
{
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void free_base_list(BaseList *list)
{
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

HRESULT add_base(BaseList *list, Base new_item)
{
    if (list->count >= list->capacity)
    {
        unsigned new_cap = (list->capacity == 0) ? 10 : list->capacity * 2;
        Base *new_items = realloc(list->items, new_cap * sizeof(Base));
        if (!new_items)
        {
            perror("Memory allocation failed");
            return E_FAIL;
        }
        list->items = new_items;
        list->capacity = new_cap;
    }

    list->items[list->count++] = new_item;
    return S_OK;
}

HRESULT handle_base(const char *record, void *data)
{
    BaseList *list = (BaseList *)data;
    Base new_base;
    InputFlags flag = T_WHITESPACE;
    int length = strlen(record);
    int label_start = 0;
    for (int i = 0; record[i] != '\0' && i < length; i++)
    {
        char c = record[i];
        char *end;
        switch (flag)
        {
        case T_WHITESPACE:

            c = tolower(c);

            switch (c)
            {
            case 'i':
                flag = T_ID;
                break;
            case 'l':
                flag = T_LABEL;
                label_start = i + 1;
                break;
            case 'c':
                flag = T_DATA;
                break;
            }

            break;
        case T_ID:
        {
            if (isspace(c) || c == '\n')
            {
                flag = T_WHITESPACE;
                break;
            }
            new_base.id = (int)strtof(&record[i], &end);
            break;
        }
        case T_LABEL:
        {
            if (isspace(c) || c == '\n')
            {
                flag = T_WHITESPACE;
                int len = i - label_start;
                if (len >= sizeof(new_base.label))
                    len = sizeof(new_base.label) - 1;
                if (len > 0)
                {
                    memcpy(new_base.label, &record[label_start], len);
                    new_base.label[len] = '\0';
                }

                break;
            }
            break;
        }
        case T_DATA:
            if (isspace(c) || c == '\n')
            {
                flag = T_WHITESPACE;
                break;
            }
            if (normstrncmp(&record[i], "LA", 2) == S_OK)
            {   
                i+=2;
                new_base.position.latitude = strtod(&record[i], &end);
                i = end - record;
            }
            else if (normstrncmp(&record[i], "LO", 2) == S_OK)
            {
                i+=2;
                new_base.position.longitude = strtod(&record[i], &end);
                i = end - record;
            }
            else if (normstrncmp(&record[i], "A", 1) == S_OK)
            {
                i+=2;
                new_base.position.altitude = strtod(&record[i], &end);
                i = end - record;
                
            }

            break;
        default:
            return S_END;
        }
    }

    if (sscanf(record, "%d %63s %lf %lf %lf",
               &new_base.id, new_base.label,
               &new_base.position.latitude,
               &new_base.position.longitude,
               &new_base.position.altitude) != 5)
    {
        fprintf(stderr, "Invalid base record: %s\n", record);
        return E_FAIL;
    }

    return add_base(list, new_base);
}

HRESULT normstrncmp(const char *record, const char *str2, int charc)
{
    for (int i = 0; record[i] != '\0' && i < charc; i++)
    {
        char str = record[i];
        char str1 = str2[i];
        NormalizeInput(&str);
        NormalizeInput(&str1);

        if (str != str1)
            return S_FALSE;

        if (charc - i > 1 && record[i + 1] == '\0')
            return S_FALSE;
    }

    return S_OK;
}
