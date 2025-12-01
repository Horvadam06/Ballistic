#ifndef STORAGE_H
#define STORAGE_H

#include "error.h"
#include "calculations.h"

//====================================
// Global Ammo type
typedef struct Ammo_t
{
    unsigned id;
    char *name; // dynamically allocated string
    double weight;
    double diameter;
    double blastRadius;
} Ammo_t;

// Ammo_t_Node for linked list of global ammo
typedef struct Ammo_t_Node
{
    Ammo_t *ammoItem;
    struct Ammo_t_Node *prev;
    struct Ammo_t_Node *next;
} Ammo_t_Node;

// Global Ammo list
typedef struct
{
    Ammo_t_Node *head;
    Ammo_t_Node *tail;
} Ammo_t_List;

extern Ammo_t_List *ammo_t_list;

//====================================

// Base-specific Ammo Stock
typedef struct Ammo_Stock
{
    Ammo_t *typePtr; // pointer to global ammo
    unsigned quantity;
    struct Ammo_Stock *next;
} Ammo_Stock;

// Ammo stock list for each Base
typedef struct
{
    Ammo_Stock *head;
    Ammo_Stock *tail;
} Ammo_Stock_List;

// Base struct
typedef struct Base
{
    unsigned id;
    char *label;      // dynamically allocated string
    Coordinate coord; // pointer to Coordinate
    Ammo_Stock_List *ammoStock;
} Base;

// Base_Node for linked list
typedef struct Base_Node
{
    Base *baseItem;
    struct Base_Node *next;
} Base_Node;

typedef struct
{
    Base_Node *head;
    Base_Node *tail;
} Base_List;

extern Base_List *base_list;

//====================================

// Target struct
typedef struct Target
{
    unsigned id;
    char *label;      // dynamically allocated string
    Coordinate coord; // pointer to Coordinate
    double area;
} Target;

// Target_Node for linked list
typedef struct Target_Node
{
    Target *targetItem;
    struct Target_Node *next;
} Target_Node;

typedef struct
{
    Target_Node *head;
    Target_Node *tail;
} Target_List;

extern Target_List *target_list;

// --- Ammo List ---
// requires a NULL valued, Ammo_t_list's address!    returns S_OK when succeded, S_FAILED when the given value is not NULL
HRESULT init_Ammo_t_List(Ammo_t_List **list);
Ammo_t_Node *add_Ammo_t(Ammo_t_List *list, Ammo_t ammo); // ordered by id
int delete_Ammo_t(Ammo_t_List *list, unsigned id);
Ammo_t_Node popFront_Ammo_t(Ammo_t_List *list);
Ammo_t_Node popBack_Ammo_t(Ammo_t_List *list);
void print_Ammo_t_List(Ammo_t_List *list);
HRESULT free_Ammo_t_List(Ammo_t_List **list);

// --- Base List ---
HRESULT init_Base_List(Base_List **list);
Base_Node *add_Base(Base_List *list, Base base); // ordered by id
int delete_Base(Base_List *list, unsigned id);
Base_Node popFront_Base(Base_List *list);
Base_Node popBack_Base(Base_List *list);
void print_Base_List(Base_List *list, PROCESS mode);
Ammo_t *GetNode_Ammot_t_List(Ammo_t_List *list, unsigned id);
HRESULT free_Base_List(Base_List **list);

// --- Ammo Stock List (per Base) ---
HRESULT init_Ammo_Stock_List(Ammo_Stock_List **list);
Ammo_Stock *add_Ammo_Stock(Ammo_Stock_List *list, Ammo_t *ammoRef, unsigned quantity); // ordered by ammo id
int delete_Ammo_Stock(Ammo_Stock_List *list, Ammo_t *ammoRef);
Ammo_Stock popFront_Ammo_Stock(Ammo_Stock_List *list);
Ammo_Stock popBack_Ammo_Stock(Ammo_Stock_List *list);
void print_Ammo_Stock_List(Ammo_Stock_List *list);
HRESULT free_Ammo_Stock_List(Ammo_Stock_List **list);

// --- Target List ---
HRESULT init_Target_List(Target_List **list);
Target_Node *add_Target(Target_List *list, Target target); // ordered by id
int delete_Target(Target_List *list, unsigned id);
Target_Node popFront_Target(Target_List *list);
Target_Node popBack_Target(Target_List *list);
void print_Target_List(Target_List *list);
HRESULT free_Target_List(Target_List **list);

typedef struct
{
    double weight;
    double diameter;
    double blastRadius;
} Ammunition;

typedef enum
{
    T_ID,
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

#endif