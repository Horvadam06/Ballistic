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

    /*
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
        {
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
            default:
                const char *p = strchr(&record[i], ' ');
                if (p)
                    i = p - record;
                break;
            }
            break;
        }
        case T_ID:
        {
            if (isspace(c) || c == '\n')
            {
                flag = T_WHITESPACE;
                break;
            }
            new_base.info.id = (int)strtof(&record[i], &end);
            break;
        }
        case T_LABEL:
        {
            if (isspace(c) || c == '\n')
            {
                flag = T_WHITESPACE;
                int len = i - label_start;
                if (len >= sizeof(new_base.info.label))
                    len = sizeof(new_base.info.label) - 1;
                if (len > 0)
                {
                    memcpy(new_base.info.label, &record[label_start], len);
                    new_base.info.label[len] = '\0';
                }

                break;
            }
            break;
        }
        case T_DATA:
        {
            if (isspace(c) || c == '\n')
            {
                flag = T_WHITESPACE;
                break;
            }
            if (norm_strncmp(&record[i], "LA", 2) == S_OK)
            {
                i += 2;
                new_base.info.position.latitude = strtod(&record[i], &end);
                i = end - record;
                flag = T_WHITESPACE;
            }
            else if (norm_strncmp(&record[i], "LO", 2) == S_OK)
            {
                i += 2;
                new_base.info.position.longitude = strtod(&record[i], &end);
                i = end - record;
                flag = T_WHITESPACE;
            }
            else if (norm_strncmp(&record[i], "A", 1) == S_OK)
            {
                i += 2;
                new_base.info.position.altitude = strtod(&record[i], &end);
                i = end - record;
                flag = T_WHITESPACE;
            }
            else
                flag = T_WHITESPACE;

            break;
        }
        default:
            return S_END;
        }
    }*/
    HRESULT hr = S_OK;
    hr = ext_basic_info(record, &new_base.info);
    if (hr != S_OK)
        return hr;

    return add_base(list, new_base);
}

HRESULT norm_strncmp(const char *record, const char *str2, int charc)
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

HRESULT ext_basic_info(const char *record, Default *info)
{
    if (!info)
        return S_OK;

    memset(info, 0, sizeof(*info));
    int been_set[5] = {0}; // store if the parameter have been set

    InputFlags flag = T_WHITESPACE;
    HRESULT hr = S_OK;
    int length = strlen(record);
    int label_start = 0;
    for (int i = 0; record[i] != '\0' && i < length; i++)
    {
        char c = record[i];
        char *end;
        switch (flag)
        {
        case T_WHITESPACE:
        {
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
            default:
                const char *p = strchr(&record[i], ' ');
                if (p)
                    i = p - record;
                break;
            }
            break;
        }
        case T_ID:
        {
            if (isspace(c) || c == '\n')
            {
                flag = T_WHITESPACE;
                break;
            }
            info->id = (int)strtof(&record[i], &end);
            been_set[T_ID] = 1;
            break;
        }
        case T_LABEL:
        {
            if (isspace(c) || c == '\n')
            {
                flag = T_WHITESPACE;
                int len = i - label_start;
                if (len >= sizeof(info->label))
                    len = sizeof(info->label) - 1;
                if (len > 0)
                {
                    memcpy(info->label, &record[label_start], len);
                    been_set[T_LABEL] = 1;
                    info->label[len] = '\0';
                }

                break;
            }
            break;
        }
        case T_DATA:
        {
            if (isspace(c) || c == '\n')
            {
                flag = T_WHITESPACE;
                break;
            }
            if (norm_strncmp(&record[i], "LA", 2) == S_OK)
            {
                i += 2;
                info->position.latitude = strtod(&record[i], &end);
                been_set[T_DATA + 0] = 1;
                i = end - record;
                flag = T_WHITESPACE;
            }
            else if (norm_strncmp(&record[i], "LO", 2) == S_OK)
            {
                i += 2;
                info->position.longitude = strtod(&record[i], &end);
                been_set[T_DATA + 1] = 1;
                i = end - record;
                flag = T_WHITESPACE;
            }
            else if (norm_strncmp(&record[i], "A", 1) == S_OK)
            {
                i += 2;
                info->position.altitude = strtod(&record[i], &end);
                been_set[T_DATA + 2] = 1;
                i = end - record;
                flag = T_WHITESPACE;
            }
            else
                flag = T_WHITESPACE;

            break;
        }
        default:
            return E_FAIL; // could only happen,
        } // if something goes very wrong
    }

    for (int i = 0; i < 5; i++)
        if (been_set[i] == 0)
            return S_PARTIAL;

    return hr;
}
