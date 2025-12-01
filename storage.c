#include "storage.h"
#include "commands.h"
#include "load.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "debugmalloc.h"

Ammo_t_List *ammo_t_list = NULL;
Base_List *base_list = NULL;
Target_List *target_list = NULL;

// ammo list functions======================================================
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
    new_ammo->ballistic_coefficient = ammo.ballistic_coefficient;
    new_ammo->blastRadius = ammo.blastRadius;
    new_ammo->muzzleVelocity = ammo.muzzleVelocity;
    new_ammo->diameter = ammo.diameter;
    new_ammo->id = ammo.id;

    size_t len = strlen(ammo.name) + 1;
    new_ammo->name = (char *)malloc(len * sizeof(char));
    if (!new_ammo->name)
    {
        free(new_ammo);
        return NULL;
    }
    memcpy(new_ammo->name, ammo.name, len);

    new_ammo->weight = ammo.weight;

    // searches for the non NULL id order, and "moves" the target there
    Ammo_t_Node *target = list->head->next;
    for (; (target != list->tail); target = target->next)
        if ((target->ammoItem && new_ammo))
            if (target->ammoItem->id > new_ammo->id)
                break;

    // insterts the new item before the target
    Ammo_t_Node *new = (Ammo_t_Node *)malloc(sizeof(Ammo_t_Node));

    new->ammoItem = new_ammo;
    new->next = target;
    new->prev = target->prev;
    target->prev->next = new;
    target->prev = new;

    return new;
}

int delete_Ammo_t(Ammo_t_List *list, unsigned id)
{
    if (!list)
        return -1;

    Ammo_t_Node *target = list->head->next;
    int no_id = 1;
    while (target != list->tail)
        if (target->ammoItem && target->ammoItem->id == id)
        {
            target->next->prev = target->prev;
            target->prev->next = target->next;

            Ammo_t_Node *tmp = target->next;

            free(target->ammoItem->name); // free the string
            free(target->ammoItem);       // free the struct
            free(target);

            target = tmp;

            no_id = 0;
        }
        else
            target = target->next;

    return (no_id) ? -1 : id;
}

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
    free(target->ammoItem->name); // free the string
    free(target->ammoItem);       // free the struct
    free(target);                 // free the node
    return value;
}

Ammo_t *GetNode_Ammot_t_List(Ammo_t_List *list, unsigned id)
{
    if (!list || (list->head->next == list->tail))
        return NULL;

    int dir = (abs(list->head->next->ammoItem->id - id) <= abs(list->tail->prev->ammoItem->id - id)) ? 0 : 1; // 0 up, 1 down

    Ammo_t_Node *target = (dir) ? list->tail : list->head;

    while (target != list->tail)
        if (target->ammoItem && target->ammoItem->id == id)
        {

            return target->ammoItem;
        }
        else
            target = target->next;

    while (target != list->head)
        if (target->ammoItem && target->ammoItem->id == id)
        {

            return target->ammoItem;
        }
        else
            target = target->prev;

    return NULL;
}

void print_Ammo_t_List(Ammo_t_List *list)
{
    if (!list)
        return;

    Ammo_t_Node *target = list->head->next;

    for (; target != list->tail; target = target->next)
        if (target->ammoItem)
            printf("id: %5u | label: %20s | weight: %10.2f kg | caliber: %10.2f mm | blast radius %10.2f m | muzzle velocity %10.3f m/s | ballistic coefficient %5.3f\n",
                   target->ammoItem->id, target->ammoItem->name, target->ammoItem->weight, target->ammoItem->diameter, target->ammoItem->blastRadius, target->ammoItem->muzzleVelocity, target->ammoItem->ballistic_coefficient);
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

    *list = NULL;

    return S_OK;
}

Ammo_t *FindAmmoByID(Ammo_t_List *list, unsigned id)
{
    if (!list || (list->head->next == list->tail))
        return NULL;

    int dir = (abs(list->head->next->ammoItem->id - id) <= abs(list->tail->prev->ammoItem->id - id)) ? 0 : 1; // 0 up, 1 down

    Ammo_t_Node *target = (dir) ? list->tail : list->head;

    while (target != list->tail)
        if (target->ammoItem && target->ammoItem->id == id)
        {

            return target->ammoItem;
        }
        else
            target = target->next;

    while (target != list->head)
        if (target->ammoItem && target->ammoItem->id == id)
        {

            return target->ammoItem;
        }
        else
            target = target->prev;

    return NULL;
}

// base list functions======================================================
HRESULT init_Base_List(Base_List **list)
{
    if (*list != NULL)
        return S_FALSE;
    Base_Node *head = (Base_Node *)malloc(sizeof(Base_Node));
    Base_Node *tail = (Base_Node *)malloc(sizeof(Base_Node));

    head->baseItem = NULL;
    head->next = tail;
    tail->baseItem = NULL;
    tail->next = NULL;

    *list = (Base_List *)malloc(sizeof(Base_List));
    (*list)->head = head;
    (*list)->tail = tail;
    return S_OK;
}
// add's an ammo type to a list, in an id order, if there are multiple one's with the same id, the places the newest at front
Base_Node *add_Base(Base_List *list, Base base)
{
    if (!list)
        return NULL;

    Base *new_base = (Base *)malloc(sizeof(Base));
    new_base->id = base.id;
    new_base->coord = base.coord;

    size_t len = strlen(base.label) + 1;
    new_base->label = (char *)malloc(len * sizeof(char));
    if (!new_base->label)
    {
        free(new_base);
        return NULL;
    }
    memcpy(new_base->label, base.label, len);
    new_base->ammoStock = base.ammoStock;

    /*
    // searches for the non NULL id order, and "moves" the target there
    Base_Node *target = list->head->next;
    for (; (target != list->tail); target = target->next)
        if ((target->baseItem && new_base))
            if (target->baseItem->id > new_base->id)
                break;

    // insterts the new item before the target
    Base_Node *new = (Base_Node *)malloc(sizeof(Base_Node));

    new->baseItem = new_base;
    new->next = target->next;
    target->next = new;
    */

    Base_Node *target = list->head;
    for (; (target->next != list->tail); target = target->next)
        if ((target->next->baseItem && new_base))
            if (target->next->baseItem->id > new_base->id)
                break;

    // insterts the new item before the target
    Base_Node *new = (Base_Node *)malloc(sizeof(Base_Node));
    new->baseItem = new_base;
    new->next = target->next;
    target->next = new;

    return new;
}

int delete_Base(Base_List *list, unsigned id)
{
    if (!list)
        return -1;

    Base_Node *target = list->head->next;
    int no_id = 1;
    while (target != list->tail)
        if (target->next->baseItem && target->next->baseItem->id == id)
        {
            Base_Node *tmp = target->next;
            target->next = target->next->next;

            free_Ammo_Stock_List(&(tmp->baseItem->ammoStock));
            free(tmp->baseItem->label); // free the string
            free(tmp->baseItem);        // free the struct
            free(tmp);

            no_id = 0;
        }
        else
            target = target->next;

    return (no_id) ? -1 : id;
}

Base_Node popFront_Base(Base_List *list)
{
    Base_Node value = {NULL, NULL};
    if (!list)
        return value;
    Base_Node *target = list->head->next;
    list->head->next = target->next;
    value = *target;
    free_Ammo_Stock_List(&(target->baseItem->ammoStock));
    free(target->baseItem->label); // free the string
    free(target->baseItem);        // free the struct
    free(target);                  // free the node
    return value;
}

Base_Node popBack_Base(Base_List *list)
{
    Base_Node value = {NULL, NULL};
    if (!list)
        return value;
    Base_Node *target = list->head->next;
    for (; target->next->next != list->tail; target = target->next)
        ;
    Base_Node *tmp = target->next;
    target->next = target->next->next;
    value = *tmp;
    free_Ammo_Stock_List(&(target->baseItem->ammoStock));
    free(tmp->baseItem->label); // free the string
    free(tmp->baseItem);        // free the struct
    free(tmp);                  // free the node
    return value;
}

void print_Base_List(Base_List *list, PROCESS mode)
{
    if (!list)
        return;

    Base_Node *target = list->head->next;

    for (; target != list->tail; target = target->next)
        if (target->baseItem)
        {
            printf("id: %5u | label: %20s | latitude: %10.3fdeg | longtitude: %10.3fdeg | altitude: %10.3fdeg\n",
                   target->baseItem->id, target->baseItem->label, target->baseItem->coord.latitude, target->baseItem->coord.longitude, target->baseItem->coord.altitude);
            if (mode == S_DEBUG)
                print_Ammo_Stock_List(target->baseItem->ammoStock);
        }
        else
            printf("NULL ammo parameter\n");
}
// lifo
HRESULT free_Base_List(Base_List **list)
{
    if (!*list)
        return S_FALSE;
    while ((*list)->head->next != (*list)->tail)
        popFront_Base(*list);

    free((*list)->head);
    free((*list)->tail);
    free(*list);

    *list = NULL;

    return S_OK;
}

Base *FindBaseByID(Base_List *list, unsigned id)
{
    if (!list || (list->head->next == list->tail))
        return NULL;

    Base_Node *target = list->head;

    while (target != list->tail)
        if (target->baseItem && target->baseItem->id == id)
        {

            return target->baseItem;
        }
        else
            target = target->next;

    return NULL;
}

// base ammo stock list functions======================================================
HRESULT init_Ammo_Stock_List(Ammo_Stock_List **list)
{
    if (*list != NULL)
        return S_FALSE;

    Ammo_Stock *head = (Ammo_Stock *)malloc(sizeof(Ammo_Stock));
    Ammo_Stock *tail = (Ammo_Stock *)malloc(sizeof(Ammo_Stock));

    head->typePtr = NULL;
    head->quantity = 0;
    head->next = tail;

    tail->typePtr = NULL;
    tail->quantity = 0;
    tail->next = NULL;

    *list = (Ammo_Stock_List *)malloc(sizeof(Ammo_Stock_List));
    (*list)->head = head;
    (*list)->tail = tail;

    return S_OK;
}
// Ordered by ammoRef->id, duplicates → newest in front
Ammo_Stock *add_Ammo_Stock(Ammo_Stock_List *list, Ammo_t *ammoRef, unsigned quantity)
{
    if (!list || !ammoRef)
        return NULL;

    // Remove existing entry to avoid duplicates (your style)
    delete_Ammo_Stock(list, ammoRef);

    // Find ordered insertion point
    Ammo_Stock *target = list->head;
    for (; target->next != list->tail; target = target->next)
        if (target->next->typePtr && ammoRef->id < target->next->typePtr->id)
            break;

    // Create new node
    Ammo_Stock *newNode = (Ammo_Stock *)malloc(sizeof(Ammo_Stock));
    newNode->typePtr = ammoRef;
    newNode->quantity = quantity;

    // Insert before target->next
    newNode->next = target->next;
    target->next = newNode;

    return newNode;
}

int delete_Ammo_Stock(Ammo_Stock_List *list, Ammo_t *ammoRef)
{
    if (!list || !ammoRef)
        return -1;

    Ammo_Stock *target = list->head;
    int found = 0;

    while (target->next != list->tail)
    {
        if (target->next->typePtr == ammoRef)
        {
            Ammo_Stock *tmp = target->next;
            target->next = target->next->next;
            free(tmp);
            found = 1;
            break; // only one possible
        }
        target = target->next;
    }

    return found ? (int)ammoRef->id : -1;
}

Ammo_Stock popFront_Ammo_Stock(Ammo_Stock_List *list)
{
    Ammo_Stock value = {NULL, 0, NULL};
    if (!list)
        return value;

    Ammo_Stock *target = list->head->next;

    // empty?
    if (target == list->tail)
        return value;

    // unlink
    list->head->next = target->next;

    // copy the struct value
    value = *target;

    // free the node itself (NOT the Ammo_t, you don't own it)
    free(target);

    return value;
}

Ammo_Stock popBack_Ammo_Stock(Ammo_Stock_List *list)
{
    Ammo_Stock value = {NULL, 0, NULL};
    if (!list)
        return value;

    Ammo_Stock *cur = list->head;

    // empty?
    if (cur->next == list->tail)
        return value;

    // walk until node BEFORE the last real node
    while (cur->next->next != list->tail)
        cur = cur->next;

    Ammo_Stock *tmp = cur->next;

    // unlink
    cur->next = tmp->next;

    // copy out the struct
    value = *tmp;

    // free only the node wrapper
    free(tmp);

    return value;
}

void print_Ammo_Stock_List(Ammo_Stock_List *list)
{
    if (!list)
        return;

    Ammo_Stock *target = list->head->next;

    for (; target != list->tail; target = target->next)
    {
        if (target->typePtr)
            printf("\tAmmo ID: %5u | Name: %-20s | Qty: %5u\n",
                   target->typePtr->id,
                   target->typePtr->name,
                   target->quantity);
        else
            printf("NULL ammo stock entry\n");
    }
    printf("\n");
}

HRESULT free_Ammo_Stock_List(Ammo_Stock_List **list)
{
    if (!*list)
        return S_FALSE;

    while ((*list)->head->next != (*list)->tail)
    {
        popFront_Ammo_Stock(*list);
    }

    free((*list)->head);
    free((*list)->tail);
    free(*list);


    *list = NULL;

    return S_OK;
}

// target list functions============================================
HRESULT init_Target_List(Target_List **list)
{
    if (*list != NULL)
        return S_FALSE;
    Target_Node *head = (Target_Node *)malloc(sizeof(Target_Node));
    Target_Node *tail = (Target_Node *)malloc(sizeof(Target_Node));

    head->targetItem = NULL;
    head->next = tail;
    tail->targetItem = NULL;
    tail->next = NULL;

    *list = (Target_List *)malloc(sizeof(Target_List));
    (*list)->head = head;
    (*list)->tail = tail;
    return S_OK;
}

Target_Node *add_Target(Target_List *list, Target target)
{
    if (!list)
        return NULL;

    Target *new_target = (Target *)malloc(sizeof(Target));
    new_target->id = target.id;
    new_target->coord = target.coord;

    size_t len = strlen(target.label) + 1;
    new_target->label = (char *)malloc(len * sizeof(char));
    if (!new_target->label)
    {
        free(new_target);
        return NULL;
    }
    new_target->area = target.area;
    memcpy(new_target->label, target.label, len);

    Target_Node *target_n = list->head;
    for (; (target_n->next != list->tail); target_n = target_n->next)
        if ((target_n->next->targetItem && new_target))
            if (target_n->next->targetItem->id > new_target->id)
                break;

    // insterts the new item before the target
    Target_Node *new = (Target_Node *)malloc(sizeof(Target_Node));
    new->targetItem = new_target;
    new->next = target_n->next;
    target_n->next = new;

    return new;
}

int delete_Target(Target_List *list, unsigned id)
{
    if (!list)
        return -1;

    Target_Node *target = list->head->next;
    int no_id = 1;
    while (target != list->tail)
        if (target->next->targetItem && target->next->targetItem->id == id)
        {
            Target_Node *tmp = target->next;
            target->next = target->next->next;

            free(tmp->targetItem->label); // free the string
            free(tmp->targetItem);        // free the struct
            free(tmp);

            no_id = 0;
        }
        else
            target = target->next;

    return (no_id) ? -1 : id;
}

Target_Node popFront_Target(Target_List *list)
{
    Target_Node value = {NULL, NULL};
    if (!list)
        return value;
    Target_Node *target = list->head->next;
    list->head->next = target->next;
    value = *target;
    free(target->targetItem->label); // free the string
    free(target->targetItem);        // free the struct
    free(target);                    // free the node
    return value;
}

Target_Node popBack_Target(Target_List *list)
{
    Target_Node value = {NULL, NULL};
    if (!list)
        return value;
    Target_Node *target = list->head->next;
    for (; target->next->next != list->tail; target = target->next)
        ;
    Target_Node *tmp = target->next;
    target->next = target->next->next;
    value = *tmp;

    free(tmp->targetItem->label); // free the string
    free(tmp->targetItem);        // free the struct
    free(tmp);                    // free the node
    return value;
}

void print_Target_List(Target_List *list)
{
    if (!list)
        return;

    Target_Node *target = list->head->next;

    for (; target != list->tail; target = target->next)
        if (target->targetItem)
        {
            printf("id: %5u | label: %20s | latitude: %10.3fdeg | longtitude: %10.3fdeg | altitude: %10.3fdeg | area: %10.3fkm^2\n",
                   target->targetItem->id, target->targetItem->label, target->targetItem->coord.latitude, target->targetItem->coord.longitude, target->targetItem->coord.altitude, target->targetItem->area);
        }
        else
            printf("NULL ammo parameter\n");
}

HRESULT free_Target_List(Target_List **list)
{
    if (!*list)
        return S_FALSE;
    while ((*list)->head->next != (*list)->tail)
        popFront_Target(*list);

    free((*list)->head);
    free((*list)->tail);
    free(*list);

    *list = NULL;

    return S_OK;
}

Target *FindTargetByID(Target_List *list, unsigned id)
{
    if (!list || (list->head->next == list->tail))
        return NULL;

    Target_Node *target = list->head;

    while (target != list->tail)
        if (target->targetItem && target->targetItem->id == id)
        {

            return target->targetItem;
        }
        else
            target = target->next;

    return NULL;
}

HRESULT Save_BaseList(Base_List *list, char *path)
{
    HRESULT hr = S_OK;
    FILE *f = fopen(path, "w");
    if (f)
    {
        Save_BaseList_file(f);
    }
    else
        hr = E_FAIL;

    fclose(f);
    return hr;
}

HRESULT Save_TargetList(Target_List *list, char *path)
{
    HRESULT hr = S_OK;
    FILE *f = fopen(path, "w");
    if (f)
    {
        Save_TargetList_file(f);
    }
    else
        hr = E_FAIL;

    fclose(f);
    return hr;
}

HRESULT Save_Ammo_t_List(Ammo_t_List *list, char *path)
{
    HRESULT hr = S_OK;
    FILE *f = fopen(path, "w");
    if (f)
    {
        Save_Ammo_t_List_file(f);
    }
    else
        hr = E_FAIL;

    fclose(f);
    return hr;
}

unsigned GetAmmoID(const Ammo_t *a)
{
    return a ? a->id : 0;
}

void Save_BaseList_file(FILE *fp)
{
    if (!fp)
    {
        fprintf(stderr, "SaveBaseRomFile: NULL file pointer\n");
        return;
    }

    fprintf(fp,
            "/*\n"
            "=====================================================\n"
            "--------------Base Parameters--------------------\n"
            "Use the following structure for inputs:\n"
            "    [I]   : id of base\n"
            "    [L]   : label (name)\n"
            "    [CLA] : latitude (degrees)\n"
            "    [CLO] : longitude (degrees)\n"
            "    [CA]  : altitude (meters)\n"
            "    [AMS] : ammo-stock map { ammo_id:count, ... }\n"
            "\n"
            "-End every line with ;\n"
            "=====================================================\n"
            "*/\n\n");

    Base_Node *node = base_list->head;

    RangeType lastRange = RANGE_NONE;

    while (node)
    {
        Base *b = node->baseItem;
        if (b)
        {
            RangeType r = GetRange(b->id);

            if (r != lastRange)
            {                            // <-- NEW RANGE DETECTED
                PrintRangeHeader(fp, r); // <-- Print header ONCE
                lastRange = r;
            }

            fprintf(fp,
                    "I%u   L%s   CLA%.6f  CLO%.6f   CA%.1f  AMS:{",
                    b->id,
                    b->label,
                    b->coord.latitude,
                    b->coord.longitude,
                    b->coord.altitude);

            Ammo_Stock *as = b->ammoStock->head;
            int first = 1;

            while (as)
            {
                unsigned ammo_id = GetAmmoID(as->typePtr);
                unsigned qty = as->quantity;

                if (!first)
                    fprintf(fp, ",");

                fprintf(fp, "%u:%u", ammo_id, qty);
                first = 0;

                as = as->next;
            }

            fprintf(fp, "};\n");
        }

        node = node->next;
    }
}

void Save_TargetList_file(FILE *fp)
{
    if (!fp)
    {
        fprintf(stderr, "TargetFile: NULL file pointer\n");
        return;
    }

    fprintf(fp,
            "/*\n"
            "=====================================================\n"
            "--------------Target Parameters--------------------\n"
            "Use the following structure for inputs:\n"
            "    [I]   : id of base\n"
            "    [L]   : label (name)\n"
            "    [CLA] : latitude (degrees)\n"
            "    [CLO] : longitude (degrees)\n"
            "    [CA]  : altitude (meters)\n"
            "    [A]   : area (km^2)\n"
            "\n"
            "-End every line with ;\n"
            "=====================================================\n"
            "*/\n\n");

    Target_Node *node = target_list->head;

    RangeType lastRange = RANGE_NONE;

    while (node)
    {
        Target *b = node->targetItem;
        if (b)
        {
            RangeType r = GetRange(b->id);

            if (r != lastRange)
            {                            // <-- NEW RANGE DETECTED
                PrintRangeHeader(fp, r); // <-- Print header ONCE
                lastRange = r;
            }

            fprintf(fp,
                    "I%u   L%s   CLA%.6f  CLO%.6f   CA%.1f  A%.3f;\n",
                    b->id,
                    b->label,
                    b->coord.latitude,
                    b->coord.longitude,
                    b->coord.altitude,
                    b->area);
        }

        node = node->next;
    }
}

void Save_Ammo_t_List_file(FILE *fp)
{
    if (!fp)
    {
        fprintf(stderr, "Ammo_t_ListFile: NULL file pointer\n");
        return;
    }

    fprintf(fp,
            "/*\n"
            "=====================================================\n"
            "--------------Ammo Parameters------------------------\n"
            "=====================================================\n"
            "Use the following structure for the inputs:\n"
            "--------The defaults---------------------------------\n"
            "\t[I]  : the id of the ammo\n"
            "\t[N]  : the name string\n"
            "\t[W]  : the weight\n"
            "\t[D]  : the diameter (caliber)\n"
            "\t[BR] : the blast radius\n"
            "\n"
            "--------The parameters-------------------------------\n"
            "(valami még lesz itt)\n"
            "=====================================================\n"
            "-The data structure has no order\n"
            "-The id gets normalized\n"
            "-End every line with ;\n"
            "=====================================================\n"
            "*/\n\n");

    Ammo_t_Node *node = ammo_t_list->head;

    while (node)
    {
        Ammo_t *b = node->ammoItem;
        if (b)
        {

            fprintf(fp,
                    "I%u   N%s   W%.3f  D%.3f   BR%.3f   MV%.3f   BC%.3f;\n",
                    b->id,
                    b->name,
                    b->weight,
                    b->diameter,
                    b->blastRadius,
                    b->muzzleVelocity,
                    b->ballistic_coefficient);
        }

        node = node->next;
    }
}

static void PrintRangeHeader(FILE *fp, RangeType r)
{
    switch (r)
    {
    case RANGE_EUROPE:
        fprintf(fp, "\n// ===================== EUROPE (IDs 0–499) =====================\n");
        break;

    case RANGE_ASIA:
        fprintf(fp, "\n// ===================== ASIA (IDs 500–999) =====================\n");
        break;

    case RANGE_AMERICAS:
        fprintf(fp, "\n// ===================== AMERICAS (IDs 1000–1499) =====================\n");
        break;

    case RANGE_AFRICA:
        fprintf(fp, "\n// ===================== AFRICA (IDs 1500–1999) =====================\n");
        break;

    case RANGE_OCEANIA:
        fprintf(fp, "\n// ===================== AUSTRALIA / OCEANIA (IDs 2000–2499) =====================\n");
        break;

    case RANGE_ANTARCTICA:
        fprintf(fp, "\n// ===================== ANTARCTICA (ID 2500) =====================\n");
        break;

    default:
        break;
    }
}
