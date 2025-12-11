#include "storage.h"
#include "commands.h"
#include "load.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "debugmalloc.h"

// Global linked list instances for the three main data types
Ammo_t_List *ammo_t_list = NULL;
Base_List *base_list = NULL;
Target_List *target_list = NULL;

// ===========================================================================
// AMMUNITION TYPE LIST FUNCTIONS
// ===========================================================================

/**
 * @brief Initializes an empty doubly-linked ammunition type list
 *
 * Creates a sentinel-based doubly-linked list with head and tail sentinel nodes.
 *
 * @param list Pointer to the list pointer (will be allocated and initialized)
 * @return HRESULT S_OK on success, S_FALSE if list already exists
 *
 * @note Head and tail sentinels always exist and contain NULL data
 * @note List structure: head <-> [data nodes] <-> tail
 * @warning Assumes *list is NULL or invalid pointer
 */
HRESULT init_Ammo_t_List(Ammo_t_List **list)
{
    if (*list)
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

/**
 * @brief Adds an ammunition type to the list in ascending ID order
 *
 * Creates a deep copy of the ammunition data and inserts it into the list
 * maintaining ascending order by ID. If multiple items with the same ID exist,
 * the newest is placed at the front of that ID group.
 *
 * @param list Pointer to the ammunition list
 * @param ammo Ammunition data to add (will be copied)
 * @return Pointer to the newly created node, or NULL on failure
 *
 * @note Performs deep copy of name string
 * @note List maintains ascending order: head <-> ... <-> new <-> ... <-> tail
 * @warning The input 'ammo' parameter is not modified or owned by the list
 */
Ammo_t_Node *add_Ammo_t(Ammo_t_List *list, Ammo_t ammo)
{
    if (!list)
        return NULL;

    // Deep copy ammunition data
    Ammo_t *new_ammo = (Ammo_t *)malloc(sizeof(Ammo_t));
    new_ammo->ballistic_coefficient = ammo.ballistic_coefficient;
    new_ammo->blastRadius = ammo.blastRadius;
    new_ammo->muzzleVelocity = ammo.muzzleVelocity;
    new_ammo->diameter = ammo.diameter;
    new_ammo->id = ammo.id;

    // Deep copy name string
    size_t len = strlen(ammo.name) + 1;
    new_ammo->name = (char *)malloc(len * sizeof(char));
    if (!new_ammo->name)
    {
        free(new_ammo);
        return NULL;
    }
    memcpy(new_ammo->name, ammo.name, len);

    new_ammo->weight = ammo.weight;

    // Find insertion point for ascending order
    Ammo_t_Node *target = list->head->next;
    for (; (target != list->tail); target = target->next)
        if ((target->ammoItem && new_ammo))
            if (target->ammoItem->id > new_ammo->id)
                break;

    // Create and insert new node
    Ammo_t_Node *new = (Ammo_t_Node *)malloc(sizeof(Ammo_t_Node));
    new->ammoItem = new_ammo;
    new->next = target;
    new->prev = target->prev;
    target->prev->next = new;
    target->prev = new;

    return new;
}

/**
 * @brief Deletes all ammunition types with the specified ID
 *
 * Searches through the list and removes all nodes containing ammunition
 * with the given ID. Frees all associated memory (node, ammo struct, name string).
 *
 * @param list Pointer to the ammunition list
 * @param id ID of ammunition to delete
 * @return The deleted ID on success, -1 if ID not found or list invalid
 *
 * @note Removes ALL occurrences of the ID (multiple can exist)
 * @warning Invalidates any existing pointers to deleted ammunition
 */
int delete_Ammo_t(Ammo_t_List *list, unsigned id)
{
    if (!list)
        return -1;

    Ammo_t_Node *target = list->head->next;
    int no_id = 1; // Track if any items were found

    while (target != list->tail)
        if (target->ammoItem && target->ammoItem->id == id)
        {
            // Remove node from list
            target->next->prev = target->prev;
            target->prev->next = target->next;

            Ammo_t_Node *tmp = target->next;

            // Free all allocated memory
            free(target->ammoItem->name); // free the string
            free(target->ammoItem);       // free the struct
            free(target);                 // free the node

            target = tmp;
            no_id = 0; // Found at least one item
        }
        else
            target = target->next;

    return (no_id) ? -1 : (int)id;
}

/**
 * @brief Removes and returns the first ammunition node from the list
 *
 * Removes the first data node (after head sentinel) and returns it.
 * The caller is responsible for the returned node's memory.
 *
 * @param list Pointer to the ammunition list
 * @return The removed node (ammoItem may be NULL if list was empty)
 *

 * @warning Returns a node with freed data if list was empty
 */
Ammo_t_Node pop_FrontAmmo_t(Ammo_t_List *list)
{
    Ammo_t_Node value = {NULL, NULL, NULL};
    if (!list)
        return value;

    Ammo_t_Node *target = list->head->next;
    list->head->next = target->next;
    target->next->prev = list->head;
    value = *target;

    // Free the node's contents (but not the node structure itself)
    free(target->ammoItem->name); // free the string
    free(target->ammoItem);       // free the struct
    free(target);                 // free the node

    return value;
}

/**
 * @brief Removes and returns the last ammunition node from the list
 *
 * Removes the last data node (before tail sentinel) and returns it.
 * The caller is responsible for the returned node's memory.
 *
 * @param list Pointer to the ammunition list
 * @return The removed node (ammoItem may be NULL if list was empty)
 *
 * @warning Returns a node with freed data if list was empty
 */
Ammo_t_Node pop_BackAmmo_t(Ammo_t_List *list)
{
    Ammo_t_Node value = {NULL, NULL, NULL};
    if (!list)
        return value;

    Ammo_t_Node *target = list->tail->prev;
    list->tail->prev = target->prev;
    target->prev->next = list->tail;
    value = *target;

    // Free the node's contents
    free(target->ammoItem->name); // free the string
    free(target->ammoItem);       // free the struct
    free(target);                 // free the node

    return value;
}

/**
 * @brief Retrieves ammunition by ID using optimized search
 *
 * Searches for ammunition with the specified ID using a bidirectional
 * search strategy that starts from whichever end is closer to the target ID.
 *
 * @param list Pointer to the ammunition list
 * @param id ID of ammunition to find
 * @return Pointer to the ammunition struct, or NULL if not found
 *
 * @note Uses bidirectional search for efficiency with sorted IDs
 * @note Returns first occurrence if multiple items share same ID
 * @warning Returns pointer to internal data - do not free
 */
Ammo_t *GetNode_Ammo_t_List(Ammo_t_List *list, unsigned id)
{
    if (!list || (list->head->next == list->tail))
        return NULL;

    // Choose search direction based on which end is closer to target ID
    int dir = (abs((int)list->head->next->ammoItem->id - (int)id) <=
               abs((int)list->tail->prev->ammoItem->id - (int)id))
                  ? 0
                  : 1; // 0 forward, 1 backward

    Ammo_t_Node *target = (dir) ? list->tail : list->head;

    // Search in chosen direction
    while (target != list->tail)
        if (target->ammoItem && target->ammoItem->id == id)
            return target->ammoItem;
        else
            target = target->next;

    // If not found, search in opposite direction
    while (target != list->head)
        if (target->ammoItem && target->ammoItem->id == id)
            return target->ammoItem;
        else
            target = target->prev;

    return NULL;
}

/**
 * @brief Prints all ammunition in the list to stdout
 *
 * Iterates through the list and displays all ammunition data in a formatted
 * table. Shows ID, name, weight, diameter, blast radius, muzzle velocity,
 * and ballistic coefficient.
 *
 * @param list Pointer to the ammunition list
 */
void print_Ammo_t_List(Ammo_t_List *list)
{
    if (!list)
        return;

    Ammo_t_Node *target = list->head->next;

    for (; target != list->tail; target = target->next)
        if (target->ammoItem)
            printf("id: %5u | label: %20s | weight: %10.2f kg | caliber: %10.2f mm | blast radius: %10.2f m | muzzle velocity %10.3f m/s | ballistic coefficient %5.3f\n",
                   target->ammoItem->id, target->ammoItem->name, target->ammoItem->weight,
                   target->ammoItem->diameter, target->ammoItem->blastRadius,
                   target->ammoItem->muzzleVelocity, target->ammoItem->ballistic_coefficient);
        else
            printf("NULL ammo parameter\n");
}

/**
 * @brief Completely frees all memory associated with an ammunition list
 *
 * Recursively removes and frees all nodes, sentinels, and the list structure itself.
 * Sets the list pointer to NULL to prevent use-after-free.
 *
 * @param list Pointer to the list pointer (will be set to NULL)
 * @return HRESULT S_OK on success, S_FALSE if list was already NULL
 *
 * @note Uses pop_BackAmmo_t() which frees node contents
 */
HRESULT free_Ammo_t_List(Ammo_t_List **list)
{
    if (!*list)
        return S_FALSE;

    // Remove all data nodes from back to front
    while ((*list)->head->next != (*list)->tail)
        pop_BackAmmo_t(*list);

    // Free sentinel nodes and list structure
    free((*list)->head);
    free((*list)->tail);
    free(*list);

    *list = NULL;

    return S_OK;
}

/**
 * @brief Finds ammunition by ID (alias for GetNode_Ammo_t_List)
 *
 * Provides a more intuitive name for the ammunition search function.
 * Uses the same optimized bidirectional search algorithm.
 *
 * @param list Pointer to the ammunition list
 * @param id ID of ammunition to find
 * @return Pointer to the ammunition struct, or NULL if not found
 *
 * @note Functionally identical to GetNode_Ammo_t_List()
 * @see GetNode_Ammo_t_List()
 */
Ammo_t *FindAmmoByID(Ammo_t_List *list, unsigned id)
{
    if (!list || (list->head->next == list->tail))
        return NULL;

    int dir = (abs((int)list->head->next->ammoItem->id - (int)id) <=
               abs((int)list->tail->prev->ammoItem->id - (int)id))
                  ? 0
                  : 1;

    Ammo_t_Node *target = (dir) ? list->tail : list->head;

    while (target != list->tail)
        if (target->ammoItem && target->ammoItem->id == id)
            return target->ammoItem;
        else
            target = target->next;

    while (target != list->head)
        if (target->ammoItem && target->ammoItem->id == id)
            return target->ammoItem;
        else
            target = target->prev;

    return NULL;
}

// ===========================================================================
// BASE LIST FUNCTIONS (Singly-linked)
// ===========================================================================

/**
 * @brief Initializes an empty singly-linked base list
 *
 * Creates a sentinel-based singly-linked list for base structures.
 * Uses head and tail sentinel nodes to simplify operations.
 *
 * @param list Pointer to the list pointer (will be allocated and initialized)
 * @return HRESULT S_OK on success, S_FALSE if list already exists
 *
 * @note Singly-linked (no prev pointers) unlike ammunition list
 * @note Structure: head -> [data nodes] -> tail -> NULL
 */
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

/**
 * @brief Adds a base to the list in ascending ID order
 *
 * Creates a deep copy of base data and inserts it maintaining ascending
 * order by ID. Also copies the ammunition stock list associated with the base.
 *
 * @param list Pointer to the base list
 * @param base Base data to add (will be copied)
 * @return Pointer to the newly created node, or NULL on failure
 *
 * @note Performs deep copy of label string and ammo stock list reference
 * @note The ammoStock pointer is copied (not deep copied)
 * @warning Input base's ammoStock must remain valid after insertion
 */
Base_Node *add_Base(Base_List *list, Base base)
{
    if (!list)
        return NULL;

    // Deep copy base data
    Base *new_base = (Base *)malloc(sizeof(Base));
    new_base->id = base.id;
    new_base->coord = base.coord;

    // Deep copy label string
    size_t len = strlen(base.label) + 1;
    new_base->label = (char *)malloc(len * sizeof(char));
    if (!new_base->label)
    {
        free(new_base);
        return NULL;
    }
    memcpy(new_base->label, base.label, len);

    // Copy ammo stock list pointer (shallow copy - list is not duplicated)
    new_base->ammoStock = base.ammoStock;

    // Find insertion point for ascending order
    Base_Node *target = list->head;
    for (; (target->next != list->tail); target = target->next)
        if ((target->next->baseItem && new_base))
            if (target->next->baseItem->id > new_base->id)
                break;

    // Create and insert new node
    Base_Node *new = (Base_Node *)malloc(sizeof(Base_Node));
    new->baseItem = new_base;
    new->next = target->next;
    target->next = new;

    return new;
}

/**
 * @brief Deletes all bases with the specified ID
 *
 * Searches through the list and removes all nodes containing bases
 * with the given ID. Frees all associated memory including ammo stock list.
 *
 * @param list Pointer to the base list
 * @param id ID of base to delete
 * @return The deleted ID on success, -1 if ID not found or list invalid
 *
 * @note Also frees the associated ammunition stock list
 * @warning Invalidates any existing pointers to deleted base
 */
int delete_Base(Base_List *list, unsigned id)
{
    if (!list)
        return -1;

    Base_Node *target = list->head;
    int no_id = 1;

    while (target->next != list->tail)
        if (target->next->baseItem && target->next->baseItem->id == id)
        {
            Base_Node *tmp = target->next;
            target->next = target->next->next;

            // Free all associated memory
            free_Ammo_Stock_List(&(tmp->baseItem->ammoStock)); // Free ammo stock list
            free(tmp->baseItem->label);                        // Free label string
            free(tmp->baseItem);                               // Free base struct
            free(tmp);                                         // Free node

            no_id = 0;
        }
        else
            target = target->next;

    return (no_id) ? -1 : (int)id;
}

/**
 * @brief Removes and returns the first base from the list
 *
 * Removes the first data node and returns it. The caller is responsible
 * for managing the returned node's memory.
 *
 * @param list Pointer to the base list
 * @return The removed node (baseItem may be NULL if list was empty)
 *
 */
Base_Node popFront_Base(Base_List *list)
{
    Base_Node value = {NULL, NULL};
    if (!list)
        return value;

    Base_Node *target = list->head->next;
    list->head->next = target->next;
    value = *target;

    // Free the node's contents
    free_Ammo_Stock_List(&(target->baseItem->ammoStock));
    free(target->baseItem->label);
    free(target->baseItem);
    free(target);

    return value;
}

/**
 * @brief Removes and returns the last base from the list
 *
 * Removes the last data node and returns it. The caller is responsible
 * for managing the returned node's memory.
 *
 * @param list Pointer to the base list
 * @return The removed node (baseItem may be NULL if list was empty)
 *
 */
Base_Node popBack_Base(Base_List *list)
{
    Base_Node value = {NULL, NULL};
    if (!list)
        return value;

    Base_Node *target = list->head->next;

    // Find second-to-last node
    for (; target->next->next != list->tail; target = target->next)
        ;

    Base_Node *tmp = target->next;
    target->next = target->next->next;
    value = *tmp;

    // Free the node's contents
    free_Ammo_Stock_List(&(tmp->baseItem->ammoStock));
    free(tmp->baseItem->label);
    free(tmp->baseItem);
    free(tmp);

    return value;
}

/**
 * @brief Prints all bases in the list to stdout
 *
 * Iterates through the list and displays all base data in a formatted table.
 * In debug mode, also prints the ammunition stock list for each base.
 *
 * @param list Pointer to the base list
 * @param mode Process mode (S_DEBUG shows ammo stock, S_RUNNING does not)
 */
void print_Base_List(Base_List *list, PROCESS mode)
{
    if (!list)
        return;

    Base_Node *target = list->head->next;

    for (; target != list->tail; target = target->next)
        if (target->baseItem)
        {
            printf("id: %5u | label: %20s | latitude: %10.3fdeg | longtitude: %10.3fdeg | altitude: %10.3fdeg\n",
                   target->baseItem->id, target->baseItem->label,
                   target->baseItem->coord.latitude, target->baseItem->coord.longitude,
                   target->baseItem->coord.altitude);

            if (mode == S_DEBUG)
                print_Ammo_Stock_List(target->baseItem->ammoStock);
        }
        else
            printf("NULL ammo parameter\n");
}

/**
 * @brief Completely frees all memory associated with a base list
 *
 * Recursively removes and frees all nodes, sentinels, and the list structure.
 * Also frees all ammunition stock lists associated with each base.
 *
 * @param list Pointer to the list pointer (will be set to NULL)
 * @return HRESULT S_OK on success, S_FALSE if list was already NULL
 *
 * @warning Invalidates all pointers to this list and its contents
 */
HRESULT free_Base_List(Base_List **list)
{
    if (!*list)
        return S_FALSE;

    // Remove all data nodes from front to back
    while ((*list)->head->next != (*list)->tail)
        popFront_Base(*list);

    // Free sentinel nodes and list structure
    free((*list)->head);
    free((*list)->tail);
    free(*list);

    *list = NULL;

    return S_OK;
}

/**
 * @brief Finds a base by ID using linear search
 *
 * Searches through the singly-linked list for a base with the specified ID.
 * Returns the first occurrence found.
 *
 * @param list Pointer to the base list
 * @param id ID of base to find
 * @return Pointer to the base struct, or NULL if not found
 *
 * @note Uses linear search (O(n)) - list is sorted but search isn't optimized
 * @warning Returns pointer to internal data - do not free
 */
Base *FindBaseByID(Base_List *list, unsigned id)
{
    if (!list || (list->head->next == list->tail))
        return NULL;

    Base_Node *target = list->head;

    while (target != list->tail)
        if (target->baseItem && target->baseItem->id == id)
            return target->baseItem;
        else
            target = target->next;

    return NULL;
}

// ===========================================================================
// AMMUNITION STOCK LIST FUNCTIONS (Base inventory)
// ===========================================================================

/**
 * @brief Initializes an empty ammunition stock list
 *
 * Creates a sentinel-based singly-linked list for tracking ammunition
 * quantities at a base. Each node references an ammunition type and quantity.
 *
 * @param list Pointer to the stock list pointer
 * @return HRESULT S_OK on success, S_FALSE if list already exists
 *
 * @note Nodes reference ammunition types (don't own them)
 * @note Structure: head -> [stock nodes] -> tail -> NULL
 */
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

/**
 * @brief Adds or updates ammunition stock in a base's inventory
 *
 * Adds ammunition to a base's stock list. If the ammunition type already exists,
 * it is removed first (replaced). Maintains ascending order by ammunition ID.
 *
 * @param list Pointer to the stock list
 * @param ammoRef Pointer to ammunition type (from global list)
 * @param quantity Number of rounds available
 * @return Pointer to the newly created stock node, or NULL on failure
 *
 * @note Removes existing entry for same ammunition type first
 * @note Maintains sorted order by ammunition ID
 * @warning ammoRef must point to valid ammunition in global list
 */
Ammo_Stock *add_Ammo_Stock(Ammo_Stock_List *list, Ammo_t *ammoRef, unsigned quantity)
{
    if (!list || !ammoRef)
        return NULL;

    // Remove existing entry to avoid duplicates
    delete_Ammo_Stock(list, ammoRef);

    // Find insertion point for ascending order by ID
    Ammo_Stock *target = list->head;
    for (; target->next != list->tail; target = target->next)
        if (target->next->typePtr && ammoRef->id < target->next->typePtr->id)
            break;

    // Create and insert new stock node
    Ammo_Stock *newNode = (Ammo_Stock *)malloc(sizeof(Ammo_Stock));
    newNode->typePtr = ammoRef;
    newNode->quantity = quantity;

    newNode->next = target->next;
    target->next = newNode;

    return newNode;
}

/**
 * @brief Removes ammunition stock from a base's inventory
 *
 * Searches for and removes the stock entry referencing the specified
 * ammunition type. Only removes the stock node, not the ammunition type itself.
 *
 * @param list Pointer to the stock list
 * @param ammoRef Pointer to ammunition type to remove from stock
 * @return The ammunition ID if found and removed, -1 otherwise
 *
 * @note Only removes the stock entry, not the ammunition type from global list
 * @note Searches by pointer equality, not by ID
 */
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
            break; // Only one possible entry per ammo type
        }
        target = target->next;
    }

    return found ? (int)ammoRef->id : -1;
}

/**
 * @brief Removes and returns the first stock item from the list
 *
 * Removes the first data node and returns it. The caller is responsible
 * for managing the returned node's memory.
 *
 * @param list Pointer to the stock list
 * @return The removed stock item (typePtr may be NULL if list was empty)
 *
 * @note Does not free the ammunition type pointer (owned by global list)
 */
Ammo_Stock popFront_Ammo_Stock(Ammo_Stock_List *list)
{
    Ammo_Stock value = {NULL, 0, NULL};
    if (!list)
        return value;

    Ammo_Stock *target = list->head->next;

    // Check if list is empty
    if (target == list->tail)
        return value;

    // Unlink node from list
    list->head->next = target->next;

    // Copy node data
    value = *target;

    // Free only the node wrapper (not the ammunition type)
    free(target);

    return value;
}

/**
 * @brief Removes and returns the last stock item from the list
 *
 * Removes the last data node and returns it. The caller is responsible
 * for managing the returned node's memory.
 *
 * @param list Pointer to the stock list
 * @return The removed stock item (typePtr may be NULL if list was empty)
 *
 */
Ammo_Stock popBack_Ammo_Stock(Ammo_Stock_List *list)
{
    Ammo_Stock value = {NULL, 0, NULL};
    if (!list)
        return value;

    Ammo_Stock *cur = list->head;

    // Check if list is empty
    if (cur->next == list->tail)
        return value;

    // Walk to node before the last real node
    while (cur->next->next != list->tail)
        cur = cur->next;

    Ammo_Stock *tmp = cur->next;

    // Unlink node
    cur->next = tmp->next;

    // Copy node data
    value = *tmp;

    // Free only the node wrapper
    free(tmp);

    return value;
}

/**
 * @brief Prints all ammunition stock items to stdout
 *
 * Displays a formatted list of ammunition types and quantities in a base's
 * inventory. Shows ID, name, and quantity for each stock item.
 *
 * @param list Pointer to the stock list
 */
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

/**
 * @brief Completely frees all memory associated with a stock list
 *
 * Recursively removes and frees all stock nodes, sentinels, and the list structure.
 * Does NOT free the ammunition type pointers (owned by global list).
 *
 * @param list Pointer to the list pointer (will be set to NULL)
 * @return HRESULT S_OK on success, S_FALSE if list was already NULL
 */
HRESULT free_Ammo_Stock_List(Ammo_Stock_List **list)
{
    if (!*list)
        return S_FALSE;

    // Remove all data nodes from front to back
    while ((*list)->head->next != (*list)->tail)
        popFront_Ammo_Stock(*list);

    // Free sentinel nodes and list structure
    free((*list)->head);
    free((*list)->tail);
    free(*list);

    *list = NULL;

    return S_OK;
}

// ===========================================================================
// TARGET LIST FUNCTIONS (Singly-linked)
// ===========================================================================

/**
 * @brief Initializes an empty singly-linked target list
 *
 * Creates a sentinel-based singly-linked list for target structures.
 * Similar structure to base list but for target data.
 *
 * @param list Pointer to the list pointer (will be allocated and initialized)
 * @return HRESULT S_OK on success, S_FALSE if list already exists
 *
 * @note Singly-linked with head and tail sentinel nodes
 */
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

/**
 * @brief Adds a target to the list in ascending ID order
 *
 * Creates a deep copy of target data and inserts it maintaining ascending
 * order by ID. Targets represent locations to be engaged.
 *
 * @param list Pointer to the target list
 * @param target Target data to add (will be copied)
 * @return Pointer to the newly created node, or NULL on failure
 *
 * @note Performs deep copy of label string
 * @note Maintains sorted order by target ID
 */
Target_Node *add_Target(Target_List *list, Target target)
{
    if (!list)
        return NULL;

    // Deep copy target data
    Target *new_target = (Target *)malloc(sizeof(Target));
    new_target->id = target.id;
    new_target->coord = target.coord;

    // Deep copy label string
    size_t len = strlen(target.label) + 1;
    new_target->label = (char *)malloc(len * sizeof(char));
    if (!new_target->label)
    {
        free(new_target);
        return NULL;
    }
    new_target->area = target.area;
    memcpy(new_target->label, target.label, len);

    // Find insertion point for ascending order
    Target_Node *target_n = list->head;
    for (; (target_n->next != list->tail); target_n = target_n->next)
        if ((target_n->next->targetItem && new_target))
            if (target_n->next->targetItem->id > new_target->id)
                break;

    // Create and insert new node
    Target_Node *new = (Target_Node *)malloc(sizeof(Target_Node));
    new->targetItem = new_target;
    new->next = target_n->next;
    target_n->next = new;

    return new;
}

/**
 * @brief Deletes all targets with the specified ID
 *
 * Searches through the list and removes all nodes containing targets
 * with the given ID. Frees all associated memory.
 *
 * @param list Pointer to the target list
 * @param id ID of target to delete
 * @return The deleted ID on success, -1 if ID not found or list invalid
 *
 * @warning Invalidates any existing pointers to deleted target
 */
int delete_Target(Target_List *list, unsigned id)
{
    if (!list)
        return -1;

    Target_Node *target = list->head;
    int no_id = 1;

    while (target->next != list->tail)
        if (target->next->targetItem && target->next->targetItem->id == id)
        {
            Target_Node *tmp = target->next;
            target->next = target->next->next;

            free(tmp->targetItem->label); // Free label string
            free(tmp->targetItem);        // Free target struct
            free(tmp);                    // Free node

            no_id = 0;
        }
        else
            target = target->next;

    return (no_id) ? -1 : (int)id;
}

/**
 * @brief Removes and returns the first target from the list
 *
 * Removes the first data node and returns it. The caller is responsible
 * for managing the returned node's memory.
 *
 * @param list Pointer to the target list
 * @return The removed node (targetItem may be NULL if list was empty)
 */
Target_Node popFront_Target(Target_List *list)
{
    Target_Node value = {NULL, NULL};
    if (!list)
        return value;

    Target_Node *target = list->head->next;
    list->head->next = target->next;
    value = *target;

    free(target->targetItem->label);
    free(target->targetItem);
    free(target);

    return value;
}

/**
 * @brief Removes and returns the last target from the list
 *
 * Removes the last data node and returns it. The caller is responsible
 * for managing the returned node's memory.
 *
 * @param list Pointer to the target list
 * @return The removed node (targetItem may be NULL if list was empty)
 */
Target_Node popBack_Target(Target_List *list)
{
    Target_Node value = {NULL, NULL};
    if (!list)
        return value;

    Target_Node *target = list->head->next;

    // Find second-to-last node
    for (; target->next->next != list->tail; target = target->next)
        ;

    Target_Node *tmp = target->next;
    target->next = target->next->next;
    value = *tmp;

    free(tmp->targetItem->label);
    free(tmp->targetItem);
    free(tmp);

    return value;
}

/**
 * @brief Prints all targets in the list to stdout
 *
 * Iterates through the list and displays all target data in a formatted table.
 * Shows ID, label, coordinates, and area for each target.
 *
 * @param list Pointer to the target list
 */
void print_Target_List(Target_List *list)
{
    if (!list)
        return;

    Target_Node *target = list->head->next;

    for (; target != list->tail; target = target->next)
        if (target->targetItem)
        {
            printf("id: %5u | label: %20s | latitude: %10.3fdeg | longtitude: %10.3fdeg | altitude: %10.3fdeg | area: %10.3fkm^2\n",
                   target->targetItem->id, target->targetItem->label,
                   target->targetItem->coord.latitude, target->targetItem->coord.longitude,
                   target->targetItem->coord.altitude, target->targetItem->area);
        }
        else
            printf("NULL ammo parameter\n");
}

/**
 * @brief Completely frees all memory associated with a target list
 *
 * Recursively removes and frees all nodes, sentinels, and the list structure.
 * Sets the list pointer to NULL to prevent use-after-free.
 *
 * @param list Pointer to the list pointer (will be set to NULL)
 * @return HRESULT S_OK on success, S_FALSE if list was already NULL
 */
HRESULT free_Target_List(Target_List **list)
{
    if (!*list)
        return S_FALSE;

    // Remove all data nodes from front to back
    while ((*list)->head->next != (*list)->tail)
        popFront_Target(*list);

    // Free sentinel nodes and list structure
    free((*list)->head);
    free((*list)->tail);
    free(*list);

    *list = NULL;

    return S_OK;
}

/**
 * @brief Finds a target by ID using linear search
 *
 * Searches through the singly-linked list for a target with the specified ID.
 * Returns the first occurrence found.
 *
 * @param list Pointer to the target list
 * @param id ID of target to find
 * @return Pointer to the target struct, or NULL if not found
 *
 * @note Uses linear search (O(n)) - list is sorted but search isn't optimized
 */
Target *FindTargetByID(Target_List *list, unsigned id)
{
    if (!list || (list->head->next == list->tail))
        return NULL;

    Target_Node *target = list->head;

    while (target != list->tail)
        if (target->targetItem && target->targetItem->id == id)
            return target->targetItem;
        else
            target = target->next;

    return NULL;
}

// ===========================================================================
// SAVE/EXPORT FUNCTIONS
// ===========================================================================

/**
 * @brief Saves the base list to a file
 *
 * Opens the specified file and writes the entire base list in the
 * application's custom data format with comments and regional headers.
 *
 * @param path File path to save to
 * @return HRESULT S_OK on success, E_FAIL on file open error
 *
 * @note Uses Save_BaseList_file() for actual writing
 * @note File is overwritten if it exists
 */
HRESULT Save_BaseList(char *path)
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

/**
 * @brief Saves the target list to a file
 *
 * Opens the specified file and writes the entire target list in the
 * application's custom data format with comments and regional headers.
 *
 * @param path File path to save to
 * @return HRESULT S_OK on success, E_FAIL on file open error
 */
HRESULT Save_TargetList(char *path)
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

/**
 * @brief Saves the ammunition type list to a file
 *
 * Opens the specified file and writes the entire ammunition type list
 * in the application's custom data format with comments.
 *
 * @param path File path to save to
 * @return HRESULT S_OK on success, E_FAIL on file open error
 */
HRESULT Save_Ammo_t_List(char *path)
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

/**
 * @brief Helper function to get ammunition ID safely
 *
 * Returns the ID of an ammunition type, or 0 if the pointer is NULL.
 * Used as a safe accessor for potentially NULL ammunition pointers.
 *
 * @param a Pointer to ammunition type (may be NULL)
 * @return Ammunition ID, or 0 if pointer is NULL
 */
unsigned GetAmmoID(const Ammo_t *a)
{
    return a ? a->id : 0;
}

/**
 * @brief Writes base list data to an already open file
 *
 * Formats and writes the entire base list to the specified file stream.
 * Includes file header comments, regional section headers, and all base data
 * in the application's custom format.
 *
 * @param fp FILE pointer to write to (must be open for writing)
 *
 * @note Groups bases by geographic region based on ID ranges
 * @note Includes ammunition stock lists for each base
 * @warning File pointer must be valid and open for writing
 */
void Save_BaseList_file(FILE *fp)
{
    if (!fp)
    {
        fprintf(stderr, "SaveBaseRomFile: NULL file pointer\n");
        return;
    }

    // Write file header with format documentation
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
            "--------The parameters-------------------------------\n"
            "    [AMS] : ammo-stock map { ammo_id:count, ... }\n"
            "=====================================================\n"
            "-The data structure has no order\n"
            "-The id gets normalized\n"
            "-End every line with ;\n"
            "=====================================================\n"
            "*/\n\n");

    Base_Node *node = base_list->head;
    RangeType lastRange = RANGE_NONE;

    // Write all bases with regional grouping
    while (node)
    {
        Base *b = node->baseItem;
        if (b)
        {
            RangeType r = GetRange(b->id);

            // Print regional header when region changes
            if (r != lastRange)
            {
                PrintRangeHeader(fp, r);
                lastRange = r;
            }

            // Write base data
            fprintf(fp,
                    "I%u   L%s   CLA%.6f  CLO%.6f   CA%.1f  AMS:{",
                    b->id,
                    b->label,
                    b->coord.latitude,
                    b->coord.longitude,
                    b->coord.altitude);

            // Write ammunition stock list
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

/**
 * @brief Writes target list data to an already open file
 *
 * Formats and writes the entire target list to the specified file stream.
 * Includes file header comments, regional section headers, and all target data.
 *
 * @param fp FILE pointer to write to (must be open for writing)
 *
 * @note Groups targets by geographic region based on ID ranges
 */
void Save_TargetList_file(FILE *fp)
{
    if (!fp)
    {
        fprintf(stderr, "TargetFile: NULL file pointer\n");
        return;
    }

    // Write file header with format documentation
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
            "--------The parameters-------------------------------\n"
            "    [A]   : area (km^2)\n"
            "=====================================================\n"
            "-The data structure has no order\n"
            "-The id gets normalized\n"
            "-End every line with ;\n"
            "=====================================================\n"
            "*/\n\n");

    Target_Node *node = target_list->head;
    RangeType lastRange = RANGE_NONE;

    // Write all targets with regional grouping
    while (node)
    {
        Target *b = node->targetItem;
        if (b)
        {
            RangeType r = GetRange(b->id);

            // Print regional header when region changes
            if (r != lastRange)
            {
                PrintRangeHeader(fp, r);
                lastRange = r;
            }

            // Write target data
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

/**
 * @brief Writes ammunition type list data to an already open file
 *
 * Formats and writes the entire ammunition type list to the specified file stream.
 * Includes file header comments and all ammunition data.
 *
 * @param fp FILE pointer to write to (must be open for writing)
 *
 * @note Ammunition types are not grouped by region (global list)
 */
void Save_Ammo_t_List_file(FILE *fp)
{
    if (!fp)
    {
        fprintf(stderr, "Ammo_t_ListFile: NULL file pointer\n");
        return;
    }

    // Write file header with format documentation
    fprintf(fp,
            "/*\n"
            "=====================================================\n"
            "--------------Ammo Parameters------------------------\n"
            "=====================================================\n"
            "Use the following structure for the inputs:\n"
            "--------The defaults---------------------------------\n"
            "\t[I]  : the id of the ammo\n"
            "\t[N]  : the name string\n"
            "--------The parameters-------------------------------\n"
            "\t[W]  : the weight\n"
            "\t[D]  : the diameter (caliber)\n"
            "\t[BR] : the blast radius\n"
            "=====================================================\n"
            "-The data structure has no order\n"
            "-The id gets normalized\n"
            "-End every line with ;\n"
            "=====================================================\n"
            "*/\n\n");

    Ammo_t_Node *node = ammo_t_list->head;

    // Write all ammunition types
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

/**
 * @brief Determines geographic region based on ID range
 *
 * Maps numeric IDs to geographic regions for organizational purposes
 * in saved files. Used to group data by continent/region.
 *
 * @param id The ID to categorize
 * @return RangeType indicating geographic region
 *
 * @note ID ranges:
 *       0-999: Europe
 *       1000-1999: Asia
 *       2000-2999: Americas
 *       3000-3999: Africa
 *       4000-4999: Oceania
 *       5000+: Antarctica
 */
RangeType GetRange(unsigned id)
{
    if (id <= 999)
        return RANGE_EUROPE;
    if (id <= 1999)
        return RANGE_ASIA;
    if (id <= 2999)
        return RANGE_AMERICAS;
    if (id <= 3999)
        return RANGE_AFRICA;
    if (id <= 4999)
        return RANGE_OCEANIA;
    if (id >= 5000)
        return RANGE_ANTARCTICA;

    return RANGE_NONE;
}

/**
 * @brief Prints a regional header comment to a file
 *
 * Writes a formatted comment header indicating a geographic region
 * section in saved data files. Helps organize data by region.
 *
 * @param fp FILE pointer to write to
 * @param r Region type to write header for
 *
 * @note Only writes header if region is valid (not RANGE_NONE)
 */
void PrintRangeHeader(FILE *fp, RangeType r)
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

/**
 * @brief Saves all data lists to their respective files
 *
 * Orchestrates the saving of all three data lists (bases, targets, ammunition)
 * to their respective files in the configured working directory.
 * Reports success/failure for each save operation.
 *
 * @return HRESULT S_OK if all saves succeeded, S_FALSE if any failed
 *
 * @note Saves to: bases.txt, targets.txt, ammo_catalouge.txt
 * @note Uses global folder_path for directory location
 * @note Continues saving even if one file fails
 */
HRESULT handle_save()
{
    HRESULT hr = S_OK;
    char folder_buffer[1024];

    // Save base list
    if (base_list)
    {
        strcpy(folder_buffer, folder_path);
        strcat(folder_buffer, "bases.txt");
        if (SUCCEEDED(Save_BaseList(folder_buffer)))
            printf("Base list saved succesfully!\n");
        else
            hr = S_FALSE;
    }

    // Save target list
    if (target_list)
    {
        strcpy(folder_buffer, folder_path);
        strcat(folder_buffer, "targets.txt");
        if (SUCCEEDED(Save_TargetList(folder_buffer)))
            printf("Target list saved succesfully!\n");
        else
            hr = S_FALSE;
    }

    // Save ammunition type list
    if (ammo_t_list)
    {
        strcpy(folder_buffer, folder_path);
        strcat(folder_buffer, "ammo_catalouge.txt");
        if (SUCCEEDED(Save_Ammo_t_List(folder_buffer)))
            printf("Ammo Type list saved succesfully!\n");
        else
            hr = S_FALSE;
    }
    printf("\n");

    return hr;
}