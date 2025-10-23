#ifndef STORAGE_H
#define STORAGE_H

#include "error.h"
#include "calculations.h"

typedef struct
{
    int id;
    char label[64];
    coordinate position;
    ammunition stock[];
} Base;

typedef struct
{
    int id;
    char label[64];
    coordinate position;
    double area;
} Target;

typedef struct {
    Base *items;
    unsigned count;
    unsigned capacity;
} BaseList;


void init_base_list(BaseList *list);
void free_base_list(BaseList *list);

HRESULT handle_base(const char *record, void *data);
HRESULT add_base(BaseList *list, Base new_item);

#endif