#ifndef STORAGE_H
#define STORAGE_H

#include "error.h"
#include "calculations.h"

/*
//====================================
//Global Ammo type
typedef struct Ammo_t {
    unsigned id;
    char* name;        // dynamically allocated string
    double weight;
    double diameter;
    double blastRadius;
} Ammo_t;

//Ammo_t_Node for linked list of global ammo
typedef struct Ammo_t_Node {
    Ammo_t* ammoItem;
    struct Ammo_t_Node* next;
} Ammo_t_Node;

//Global Ammo list
typedef struct {
    Ammo_t_Node* head;
    Ammo_t_Node* tail;
} Ammo_t_List;

//====================================

//Default_i (shared by Base and Target)
typedef struct {
    unsigned id;
    char* label;        // dynamically allocated string
    Coordinate* coord;  // pointer to Coordinate
} Default_i;

//====================================

//Base-specific Ammo Stock
typedef struct Ammo_Stock {
    Ammo_t* typeRef;           // pointer to global ammo
    unsigned quantity;
    struct Ammo_Stock* next;
} Ammo_Stock;

//Ammo stock list for each Base
typedef struct {
    Ammo_Stock* head;
    Ammo_Stock* tail;
} Ammo_Stock_List;

//Base struct
typedef struct Base {
    Default_i* info;
    Ammo_Stock_List* ammoStock;
} Base;

//Base_Node for linked list
typedef struct Base_Node {
    Base* baseItem;
    struct Base_Node* next;
} Base_Node;

typedef struct {
    Base_Node* head;
    Base_Node* tail;
} Base_List;

//====================================

//Target struct
typedef struct Target {
    Default_i* info;
    double area;
} Target;

//Target_Node for linked list
typedef struct Target_Node {
    Target* targetItem;
    struct Target_Node* next;
} Target_Node;

typedef struct {
    Target_Node* head;
    Target_Node* tail;
} Target_List;



// --- Ammo List ---
void initAmmo_t_List(Ammo_t_List* list);
Ammo_t_Node* addAmmo_t(Ammo_t_List* list, Ammo_t* ammo);       // ordered by id
int deleteAmmo_t(Ammo_t_List* list, unsigned id);
Ammo_t_Node* popFrontAmmo_t(Ammo_t_List* list);
Ammo_t_Node* popBackAmmo_t(Ammo_t_List* list);
void freeAmmo_t_List(Ammo_t_List* list);

// --- Base List ---
void initBase_List(Base_List* list);
Base_Node* addBase(Base_List* list, Base* base);               // ordered by id
int deleteBase(Base_List* list, unsigned id);
Base_Node* popFrontBase(Base_List* list);
Base_Node* popBackBase(Base_List* list);
void freeBase_List(Base_List* list);

// --- Ammo Stock List (per Base) ---
void initAmmo_Stock_List(Ammo_Stock_List* list);
Ammo_Stock* addAmmo_Stock(Ammo_Stock_List* list, Ammo_t* ammoRef, unsigned quantity); // ordered by ammo id
int deleteAmmo_Stock(Ammo_Stock_List* list, Ammo_t* ammoRef);
Ammo_Stock* popFrontAmmo_Stock(Ammo_Stock_List* list);
Ammo_Stock* popBackAmmo_Stock(Ammo_Stock_List* list);
void freeAmmo_Stock_List(Ammo_Stock_List* list);

// --- Target List ---
void initTarget_List(Target_List* list);
Target_Node* addTarget(Target_List* list, Target* target);     // ordered by id
int deleteTarget(Target_List* list, unsigned id);
Target_Node* popFrontTarget(Target_List* list);
Target_Node* popBackTarget(Target_List* list);
void freeTarget_List(Target_List* list);
*/
//old one for idk

typedef struct
{
    int id;
    char label[64];
    Coordinate position;
} Default_i;

typedef struct
{
    double weight;
    double diameter;
    double blastRadius;
} Ammunition;

typedef struct
{
    Default_i info;
    Ammunition stock[];
} Base;

typedef struct
{
    Default_i info;
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

//input string; constant input value; length of constant
HRESULT norm_strncmp(const char *record, const char *str2); // normalized strncmp
HRESULT ext_basic_info(const char *record, Default_i *info, int *error_flags);

#endif