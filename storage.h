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
    T_ID ,
    T_LABEL,
    T_DATA,
    T_WHITESPACE
} InputFlags;

typedef enum
{
    F_ID = 0,
    F_LABEL = 1,
    F_LONGITUDE = 2,
    F_LATITUDE = 3,
    F_ALTITUDE = 4
} Default_flags;

void init_base_list(BaseList *list);
void free_base_list(BaseList *list);

HRESULT handle_base(const char *record, void *data, int *error_flags);
HRESULT add_base(BaseList *list, Base new_item, HRESULT hr);

HRESULT norm_strncmp(const char *record, const char *str2, int charc); // normalized strncmp
HRESULT ext_basic_info(const char *record, Default *info, int *error_flags);

#endif