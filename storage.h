#ifndef STORAGE_H
#define STORAGE_H

#include "error.h"
#include "calculations.h"

typedef struct
{
    int id;
    char label[64];
    coordinate position;
} Default;

typedef struct
{
    Default info;
    ammunition stock[];
} Base;

typedef struct
{
    Default info;
    double area;
} Target;

typedef struct
{
    Base *items;
    unsigned count;
    unsigned capacity;
} BaseList;

typedef enum
{
    T_ID = 0,
    T_LABEL = 1,
    T_DATA = 2,
    T_WHITESPACE
} InputFlags;

void init_base_list(BaseList *list);
void free_base_list(BaseList *list);

HRESULT handle_base(const char *record, void *data);
HRESULT add_base(BaseList *list, Base new_item);

HRESULT norm_strncmp(const char *record, const char *str2, int charc); // normalized strncmp
HRESULT ext_basic_info(const char *record, Default *info);

#endif