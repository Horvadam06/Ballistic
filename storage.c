#include "storage.h"
#include "commands.h"
#include "load.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

Ammo_t_List *ammo_t_list = NULL;

void init_base_list(BaseList *list)
{
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void free_base_list(BaseList *list)
{
    if (!list)
        return;
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    printf("Memory cleared succesfully!\n");
}

HRESULT add_base(BaseList *list, Base new_item, HRESULT hr)
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
    return hr;
}

HRESULT handle_base(char *record, void *data, int *error_flags)
{
    BaseList *list = (BaseList *)data;
    Base new_base;

    HRESULT hr = S_OK;

    hr = ext_basic_info(record, &new_base.info, error_flags);

    if (hr == S_FALSE)
        return hr;

    return add_base(list, new_base, hr);
}

HRESULT norm_strncmp(const char *record, const char *str2)
{
    size_t length = strlen(str2);
    for (int i = 0; record[i] != '\0' && i < length; i++)
    {
        char str = record[i];
        char str1 = str2[i];
        NormalizeInput(&str);
        NormalizeInput(&str1);

        if (str != str1)
            return S_FALSE;

        if (length - i > 1 && record[i + 1] == '\0')
            return S_FALSE;
    }

    return S_OK;
}

HRESULT ext_basic_info(const char *record, Default_i *info, int *error_flags)
{
    if (!info)
        return S_OK;

    // 0's out the the info memory area
    memset(info, 0, sizeof(*info));

    InputFlags flag = T_WHITESPACE;
    HRESULT hr = S_OK;
    int length = strlen(record);
    int label_start = 0;
    for (int i = 0; record[i] != '\0' && i < length; i++)
    {
        char c = record[i];
        char next_c = (record[i + 1]) ? record[i + 1] : '\0';
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
            if (isspace(c) || c == '\n' || c == '\0')
            {
                flag = T_WHITESPACE;
                break;
            }
            info->id = (int)strtof(&record[i], &end);

            error_flags[F_ID] = 1;
            break;
        }
        case T_LABEL:
        {
            if (isspace(c) || c == '\n' || next_c == '\0')
            {
                flag = T_WHITESPACE;
                int len = (next_c == '\0') ? i - label_start + 1 : i - label_start; // if the loop exit's with \0, there is going to be a missing charcter, so it's needed for compensation
                if (len >= sizeof(info->label))
                    len = sizeof(info->label) - 1;
                if (len > 0)
                {
                    memcpy(info->label, &record[label_start], len);
                    error_flags[F_LABEL] = 1;
                    info->label[len] = '\0';
                }

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
            if (norm_strncmp(&record[i], "LA") == S_OK)
            {
                i += 2;
                info->position.latitude = strtod(&record[i], &end);
                error_flags[F_LATITUDE] = 1;
                i = end - record;
                flag = T_WHITESPACE;
            }
            else if (norm_strncmp(&record[i], "LO") == S_OK)
            {
                i += 2;
                info->position.longitude = strtod(&record[i], &end);
                error_flags[F_LONGITUDE] = 1;
                i = end - record;
                flag = T_WHITESPACE;
            }
            else if (norm_strncmp(&record[i], "A") == S_OK)
            {
                i += 2;
                info->position.altitude = strtod(&record[i], &end);
                error_flags[F_ALTITUDE] = 1;
                i = end - record;
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

    for (int i = 0; i < 5; i++)
        if (error_flags[i] != 1)
            return S_PARTIAL;

    return hr;
}

HRESULT init_Ammo_t_List(Ammo_t_List **list)
{
    if (*list != NULL)
        return S_FALSE;
    Ammo_t_Node *head = (Ammo_t_Node *)malloc(sizeof(Ammo_t_Node));
    Ammo_t_Node *tail = (Ammo_t_Node *)malloc(sizeof(Ammo_t_Node));

    head->ammoItem = NULL;
    head->prev = NULL;
    head->next = tail;
    tail->ammoItem = NULL;
    tail->next = NULL;
    tail->prev = head;

    *list = (Ammo_t_List *)malloc(sizeof(Ammo_t_List));
    (*list)->head = head;
    (*list)->tail = tail;
    return S_OK;
}

// add's an ammo type to a list, in an id order, if there are multiple one's with the same id, the places the newest at front
Ammo_t_Node *add_Ammo_t(Ammo_t_List *list, Ammo_t ammo)
{
    if (!list)
        return NULL;

    Ammo_t *new_ammo = (Ammo_t *)malloc(sizeof(Ammo_t));
    new_ammo->blastRadius = ammo.blastRadius;
    new_ammo->diameter = ammo.diameter;
    new_ammo->id = ammo.id;

    size_t len = strlen(ammo.name) + 1;
    new_ammo->name = malloc(len);
    if (!new_ammo->name)
    {
        free(new_ammo);
        return NULL;
    }
    memcpy(new_ammo->name, ammo.name, len);

    new_ammo->weight = ammo.weight;

    // searches for the non NULL id order, and "moves" the target there
    Ammo_t_Node *target = list->head;
    for (; (target->next != list->tail); target = target->next)
        if ((target->ammoItem && new_ammo))
            if (target->ammoItem->id < new_ammo->id)
                break;

    // insterts the new item before the target
    Ammo_t_Node *new = (Ammo_t_Node *)malloc(sizeof(Ammo_t_Node));

    new->next = target->next;
    new->prev = target;
    target->next = new;
    new->next->prev = new;
    new->ammoItem = new_ammo;

    return new;
}

int delete_Ammo_t(Ammo_t_List *list, unsigned id);

Ammo_t_Node pop_FrontAmmo_t(Ammo_t_List *list)
{
    Ammo_t_Node value = {NULL, NULL, NULL};
    if (!list)
        return value;
    Ammo_t_Node *target = list->head->next;
    list->head->next = target->next;
    target->next->prev = list->head;
    value = *target;
    free(target->ammoItem->name); // free the string
    free(target->ammoItem);       // free the struct
    free(target);                 // free the node
    return value;
}

Ammo_t_Node pop_BackAmmo_t(Ammo_t_List *list)
{
    Ammo_t_Node value = {NULL, NULL, NULL};
    if (!list)
        return value;
    Ammo_t_Node *target = list->tail->prev;
    list->tail->prev = target->prev;
    target->prev->next = list->tail;
    value = *target;
    free(target->ammoItem);
    free(target);
    return value;
}

void print_Ammo_t_List(Ammo_t_List *list)
{
    if (!list)
        return;

    Ammo_t_Node *target = list->head->next;

    for (; target != list->tail; target = target->next)
        if (target->ammoItem)
            printf("id: %5u | label: %10s\n", target->ammoItem->id, target->ammoItem->name);
        else
            printf("NULL ammo parameter\n");
}
// lifo
HRESULT free_Ammo_t_List(Ammo_t_List **list)
{
    if (!*list)
        return S_FALSE;
    while ((*list)->head->next != (*list)->tail)
        pop_BackAmmo_t(*list);

    free((*list)->head);
    free((*list)->tail);
    free(*list);

    return S_OK;
}