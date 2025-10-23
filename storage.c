#include "storage.h"
#include "stdio.h"
#include "stdlib.h"

void init_base_list(BaseList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void free_base_list(BaseList *list) {
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

HRESULT add_base(BaseList *list, Base new_item) {
    if (list->count >= list->capacity) {
        unsigned new_cap = (list->capacity == 0) ? 10 : list->capacity * 2;
        Base *new_items = realloc(list->items, new_cap * sizeof(Base));
        if (!new_items) {
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


