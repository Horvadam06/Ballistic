/**
 * @defgroup Storage Data Storage Structures
 * @brief Core data structures and linked list implementations
 * 
 * This module contains all data structures for storing bases, targets,
 * ammunition types, and their relationships. It implements sentinel-linked
 * lists with head and tail nodes for efficient insertion and traversal.
 * 
 * ## Data Structure Hierarchy
 * 
 * @dot
 * digraph storage_hierarchy {
 *     rankdir=TB;
 *     node [shape=record, fontname=Helvetica, fontsize=10];
 *     
 *     Ammo_t_List [label="Ammo_t_List|head* : Ammo_t_Node\ltail* : Ammo_t_Node"];
 *     Ammo_t_Node [label="Ammo_t_Node|prev* : Ammo_t_Node\lammoItem* : Ammo_t\lnext* : Ammo_t_Node"];
 *     Ammo_t [label="Ammo_t|id : unsigned\lname* : char\lweight : double\ldiameter : double\lblastRadius : double\lmuzzleVelocity : double\lballistic_coefficient : double"];
 *     
 *     Base_List [label="Base_List|head* : Base_Node\ltail* : Base_Node"];
 *     Base_Node [label="Base_Node|baseItem* : Base\lnext* : Base_Node"];
 *     Base [label="Base|id : unsigned\llabel* : char\lcoord : Coordinate\lammoStock* : Ammo_Stock_List"];
 *     Coordinate [label="Coordinate|latitude : double\llongitude : double\laltitude : double"];
 *     Ammo_Stock_List [label="Ammo_Stock_List|head* : Ammo_Stock\ltail* : Ammo_Stock"];
 *     Ammo_Stock [label="Ammo_Stock|typePtr* : Ammo_t\lquantity : unsigned\lnext* : Ammo_Stock"];
 *     
 *     Target_List [label="Target_List|head* : Target_Node\ltail* : Target_Node"];
 *     Target_Node [label="Target_Node|targetItem* : Target\lnext* : Target_Node"];
 *     Target [label="Target|id : unsigned\llabel* : char\lcoord : Coordinate\larea : double"];
 *     
 *     // Ammo relationships
 *     Ammo_t_List -> Ammo_t_Node [label="contains"];
 *     Ammo_t_Node -> Ammo_t [label="points to"];
 *     
 *     // Base relationships
 *     Base_List -> Base_Node [label="contains"];
 *     Base_Node -> Base [label="points to"];
 *     Base -> Coordinate [label="has"];
 *     Base -> Ammo_Stock_List [label="has"];
 *     Ammo_Stock_List -> Ammo_Stock [label="contains"];
 *     Ammo_Stock -> Ammo_t [label="references"];
 *     
 *     // Target relationships
 *     Target_List -> Target_Node [label="contains"];
 *     Target_Node -> Target [label="points to"];
 *     Target -> Coordinate [label="has"];
 *     
 *     // List relationships
 *     {rank=same; Ammo_t_List; Base_List; Target_List}
 * }
 * @enddot
 * 
 * ## Data File Formats
 * 
 * ### Ammunition Catalog (ammo_catalouge.txt)
 * ```
 * I0   NHE   W50.0  D120.0   BR10.0   MV800.0   BC0.5;
 * I1   NAP   W40.0  D105.0   BR8.0    MV850.0   BC0.48;
 * ```
 * 
 * ### Bases (bases.txt)
 * ```
 * I0   LBase1   CLA47.4729283  CLO19.0531429   CA100.0  AMS:{0:100,1:50};
 * I1   LBase2   CLA48.123456   CLO20.987654    CA150.0  AMS:{0:75};
 * ```
 * 
 * ### Targets (targets.txt)
 * ```
 * I0   LTarget1   CLA47.5  CLO19.1   CA50.0  A1.5;
 * I1   LTarget2   CLA48.0  CLO20.0   CA75.0  A2.0;
 * ```
 * 
 * ## Memory Management
 * 
 * All dynamically allocated strings (name, label) are freed automatically
 * when lists are destroyed. The system uses sentinel nodes (head/tail) to
 * simplify boundary conditions in list operations.
 * 
 * @{
 */

#ifndef STORAGE_H
#define STORAGE_H

#include "error.h"
#include <stdio.h>

/**
 * @brief Regional categorization based on ID ranges
 *
 * Used for organizing entities geographically and for region-aware
 * base selection algorithms.
 */
typedef enum
{
    RANGE_NONE = 0,  ///< No region assigned
    RANGE_EUROPE,    ///< IDs 0-999
    RANGE_ASIA,      ///< IDs 1000-1999
    RANGE_AMERICAS,  ///< IDs 2000-2999
    RANGE_AFRICA,    ///< IDs 3000-3999
    RANGE_OCEANIA,   ///< IDs 4000-4999
    RANGE_ANTARCTICA ///< IDs 5000+
} RangeType;

/**
 * @brief Determine region based on entity ID
 * @param id Entity identifier
 * @return Corresponding RangeType
 * @ingroup Storage
 */
RangeType GetRange(unsigned id);

/**
 * @brief Geographic coordinate structure
 * @ingroup Storage
 */
typedef struct
{
    double latitude;  ///< Latitude in degrees (-90 to 90)
    double longitude; ///< Longitude in degrees (-180 to 180)
    double altitude;  ///< Altitude in meters above WGS84 ellipsoid
} Coordinate;

/**
 * @brief Ammunition type definition
 *
 * Contains physical and ballistic properties of ammunition types.
 * These are globally defined and referenced by bases' stock lists.
 * @ingroup Storage
 */
typedef struct Ammo_t
{
    unsigned id;                  ///< Unique identifier
    char *name;                   ///< Dynamically allocated name string
    double weight;                ///< Mass in kilograms
    double diameter;              ///< Caliber in millimeters
    double blastRadius;           ///< Explosive effect radius in meters
    double muzzleVelocity;        ///< Initial velocity in m/s
    double ballistic_coefficient; ///< G7 ballistic coefficient (kg/m²)
} Ammo_t;

/**
 * @brief Node for global ammunition type linked list
 * @ingroup Storage
 */
typedef struct Ammo_t_Node
{
    Ammo_t *ammoItem;         ///< Pointer to ammunition data
    struct Ammo_t_Node *prev; ///< Previous node in list
    struct Ammo_t_Node *next; ///< Next node in list
} Ammo_t_Node;

/**
 * @brief Global ammunition type list (sorted by ID)
 *
 * Sentinel-linked list with head and tail nodes.
 * @ingroup Storage
 */
typedef struct
{
    Ammo_t_Node *head; ///< Sentinel head node (ammoItem = NULL)
    Ammo_t_Node *tail; ///< Sentinel tail node (ammoItem = NULL)
} Ammo_t_List;

/**
 * @brief Base ammunition stock entry
 *
 * Links to global ammunition type and stores quantity.
 * @ingroup Storage
 */
typedef struct Ammo_Stock
{
    Ammo_t *typePtr;         ///< Pointer to global ammunition type
    unsigned quantity;       ///< Available quantity at base
    struct Ammo_Stock *next; ///< Next stock entry in list
} Ammo_Stock;

/**
 * @brief Ammunition stock list for a single base (sorted by ammo ID)
 * @ingroup Storage
 */
typedef struct
{
    Ammo_Stock *head; ///< Sentinel head node (typePtr = NULL)
    Ammo_Stock *tail; ///< Sentinel tail node (typePtr = NULL)
} Ammo_Stock_List;

/**
 * @brief Military base structure
 *
 * Contains base identification, location, and ammunition inventory.
 * @ingroup Storage
 */
typedef struct Base
{
    unsigned id;                ///< Unique identifier
    char *label;                ///< Dynamically allocated base name
    Coordinate coord;           ///< Geographic coordinates
    Ammo_Stock_List *ammoStock; ///< Inventory of ammunition types
} Base;

/**
 * @brief Node for base linked list
 * @ingroup Storage
 */
typedef struct Base_Node
{
    Base *baseItem;         ///< Pointer to base data
    struct Base_Node *next; ///< Next base in list
} Base_Node;

/**
 * @brief Base list (sorted by ID)
 * @ingroup Storage
 */
typedef struct
{
    Base_Node *head; ///< Sentinel head node (baseItem = NULL)
    Base_Node *tail; ///< Sentinel tail node (baseItem = NULL)
} Base_List;

/**
 * @brief Target structure
 *
 * Contains target identification, location, and area.
 * @ingroup Storage
 */
typedef struct Target
{
    unsigned id;      ///< Unique identifier
    char *label;      ///< Dynamically allocated target name
    Coordinate coord; ///< Geographic coordinates
    double area;      ///< Target area in square kilometers
} Target;

/**
 * @brief Node for target linked list
 * @ingroup Storage
 */
typedef struct Target_Node
{
    Target *targetItem;       ///< Pointer to target data
    struct Target_Node *next; ///< Next target in list
} Target_Node;

/**
 * @brief Target list (sorted by ID)
 * @ingroup Storage
 */
typedef struct
{
    Target_Node *head; ///< Sentinel head node (targetItem = NULL)
    Target_Node *tail; ///< Sentinel tail node (targetItem = NULL)
} Target_List;

// Global lists (defined in storage.c)
extern Ammo_t_List *ammo_t_list; ///< Global ammunition type database
extern Base_List *base_list;     ///< All military bases
extern Target_List *target_list; ///< All targets

/**
 * @name Ammunition List Functions
 * @{
 */

/**
 * @brief Initialize an empty ammunition type list
 * @param list Double pointer to Ammo_t_List
 * @return S_OK on success, S_FALSE if list already initialized
 * @ingroup Storage
 */
P_HRESULT init_Ammo_t_List(Ammo_t_List **list);

/**
 * @brief Add ammunition type to list (sorted by ID)
 * @param list Pointer to ammunition list
 * @param ammo Ammunition data to add
 * @return Pointer to new node, NULL on error
 * @ingroup Storage
 */
Ammo_t_Node *add_Ammo_t(Ammo_t_List *list, Ammo_t ammo);

/**
 * @brief Delete ammunition type by ID
 * @param list Pointer to ammunition list
 * @param id ID of ammunition to delete
 * @return ID if found and deleted, -1 if not found
 * @ingroup Storage
 */
int delete_Ammo_t(Ammo_t_List *list, unsigned id);

/**
 * @brief Remove and return first ammunition from list
 * @param list Pointer to ammunition list
 * @return Copy of removed node (empty if list was empty)
 * @ingroup Storage
 */
Ammo_t_Node popFront_Ammo_t(Ammo_t_List *list);

/**
 * @brief Remove and return last ammunition from list
 * @param list Pointer to ammunition list
 * @return Copy of removed node (empty if list was empty)
 * @ingroup Storage
 */
Ammo_t_Node popBack_Ammo_t(Ammo_t_List *list);

/**
 * @brief Print all ammunition types to stdout
 * @param list Pointer to ammunition list
 * @ingroup Storage
 */
void print_Ammo_t_List(Ammo_t_List *list);

/**
 * @brief Free all memory associated with ammunition list
 * @param list Double pointer to ammunition list
 * @return S_OK on success, S_FALSE if list was NULL
 * @ingroup Storage
 */
P_HRESULT free_Ammo_t_List(Ammo_t_List **list);

/**
 * @brief Find ammunition by ID
 * @param list Pointer to ammunition list
 * @param id ID to search for
 * @return Pointer to ammunition, NULL if not found
 * @ingroup Storage
 */
Ammo_t *FindAmmoByID(Ammo_t_List *list, unsigned id);

/** @} */

/**
 * @name Base List Functions
 * @{
 */

/**
 * @brief Initialize an empty base list
 * @param list Double pointer to Base_List
 * @return S_OK on success, S_FALSE if list already initialized
 * @ingroup Storage
 */
P_HRESULT init_Base_List(Base_List **list);

/**
 * @brief Add base to list (sorted by ID)
 * @param list Pointer to base list
 * @param base Base data to add
 * @return Pointer to new node, NULL on error
 * @ingroup Storage
 */
Base_Node *add_Base(Base_List *list, Base base);

/**
 * @brief Delete base by ID
 * @param list Pointer to base list
 * @param id ID of base to delete
 * @return ID if found and deleted, -1 if not found
 * @ingroup Storage
 */
int delete_Base(Base_List *list, unsigned id);

/**
 * @brief Remove and return first base from list
 * @param list Pointer to base list
 * @return Copy of removed node (empty if list was empty)
 * @ingroup Storage
 */
Base_Node popFront_Base(Base_List *list);

/**
 * @brief Remove and return last base from list
 * @param list Pointer to base list
 * @return Copy of removed node (empty if list was empty)
 * @ingroup Storage
 */
Base_Node popBack_Base(Base_List *list);

/**
 * @brief Print all bases to stdout
 * @param list Pointer to base list
 * @param mode Process mode (S_DEBUG shows ammunition stock details)
 * @ingroup Storage
 */
void print_Base_List(Base_List *list, PROCESS mode);

/**
 * @brief Get ammunition type by ID from ammunition list
 * @param list Pointer to ammunition list
 * @param id ID to search for
 * @return Pointer to ammunition, NULL if not found
 * @ingroup Storage
 */
Ammo_t *GetNode_Ammo_t_List(Ammo_t_List *list, unsigned id);

/**
 * @brief Free all memory associated with base list
 * @param list Double pointer to base list
 * @return S_OK on success, S_FALSE if list was NULL
 * @ingroup Storage
 */
P_HRESULT free_Base_List(Base_List **list);

/**
 * @brief Find base by ID
 * @param list Pointer to base list
 * @param id ID to search for
 * @return Pointer to base, NULL if not found
 * @ingroup Storage
 */
Base *FindBaseByID(Base_List *list, unsigned id);

/** @} */

/**
 * @name Ammunition Stock List Functions
 * @{
 */

/**
 * @brief Initialize an empty ammunition stock list
 * @param list Double pointer to Ammo_Stock_List
 * @return S_OK on success, S_FALSE if list already initialized
 * @ingroup Storage
 */
P_HRESULT init_Ammo_Stock_List(Ammo_Stock_List **list);

/**
 * @brief Add ammunition to base stock (sorted by ammo ID)
 * @param list Pointer to stock list
 * @param ammoRef Pointer to global ammunition type
 * @param quantity Amount to add
 * @return Pointer to new stock entry, NULL on error
 * @ingroup Storage
 */
Ammo_Stock *add_Ammo_Stock(Ammo_Stock_List *list, Ammo_t *ammoRef, unsigned quantity);

/**
 * @brief Delete ammunition from stock by reference
 * @param list Pointer to stock list
 * @param ammoRef Pointer to ammunition type to remove
 * @return Ammunition ID if found and deleted, -1 if not found
 * @ingroup Storage
 */
int delete_Ammo_Stock(Ammo_Stock_List *list, Ammo_t *ammoRef);

/**
 * @brief Remove and return first ammunition from stock
 * @param list Pointer to stock list
 * @return Copy of removed stock entry (empty if list was empty)
 * @ingroup Storage
 */
Ammo_Stock popFront_Ammo_Stock(Ammo_Stock_List *list);

/**
 * @brief Remove and return last ammunition from stock
 * @param list Pointer to stock list
 * @return Copy of removed stock entry (empty if list was empty)
 * @ingroup Storage
 */
Ammo_Stock popBack_Ammo_Stock(Ammo_Stock_List *list);

/**
 * @brief Print ammunition stock to stdout
 * @param list Pointer to stock list
 * @ingroup Storage
 */
void print_Ammo_Stock_List(Ammo_Stock_List *list);

/**
 * @brief Free all memory associated with ammunition stock list
 * @param list Double pointer to stock list
 * @return S_OK on success, S_FALSE if list was NULL
 * @ingroup Storage
 */
P_HRESULT free_Ammo_Stock_List(Ammo_Stock_List **list);

/** @} */

/**
 * @name Target List Functions
 * @{
 */

/**
 * @brief Initialize an empty target list
 * @param list Double pointer to Target_List
 * @return S_OK on success, S_FALSE if list already initialized
 * @ingroup Storage
 */
P_HRESULT init_Target_List(Target_List **list);

/**
 * @brief Add target to list (sorted by ID)
 * @param list Pointer to target list
 * @param target Target data to add
 * @return Pointer to new node, NULL on error
 * @ingroup Storage
 */
Target_Node *add_Target(Target_List *list, Target target);

/**
 * @brief Delete target by ID
 * @param list Pointer to target list
 * @param id ID of target to delete
 * @return ID if found and deleted, -1 if not found
 * @ingroup Storage
 */
int delete_Target(Target_List *list, unsigned id);

/**
 * @brief Remove and return first target from list
 * @param list Pointer to target list
 * @return Copy of removed node (empty if list was empty)
 * @ingroup Storage
 */
Target_Node popFront_Target(Target_List *list);

/**
 * @brief Remove and return last target from list
 * @param list Pointer to target list
 * @return Copy of removed node (empty if list was empty)
 * @ingroup Storage
 */
Target_Node popBack_Target(Target_List *list);

/**
 * @brief Print all targets to stdout
 * @param list Pointer to target list
 * @ingroup Storage
 */
void print_Target_List(Target_List *list);

/**
 * @brief Free all memory associated with target list
 * @param list Double pointer to target list
 * @return S_OK on success, S_FALSE if list was NULL
 * @ingroup Storage
 */
P_HRESULT free_Target_List(Target_List **list);

/**
 * @brief Find target by ID
 * @param list Pointer to target list
 * @param id ID to search for
 * @return Pointer to target, NULL if not found
 * @ingroup Storage
 */
Target *FindTargetByID(Target_List *list, unsigned id);

/** @} */

/**
 * @name File I/O Functions
 * @{
 */

/**
 * @brief Get ammunition ID (safe wrapper)
 * @param a Pointer to ammunition
 * @return ID if ammunition exists, 0 otherwise
 * @ingroup Storage
 */
unsigned GetAmmoID(const Ammo_t *a);

/**
 * @brief Save base list to open file
 * @param fp Open FILE pointer for writing
 * @ingroup Storage
 */
void Save_BaseList_file(FILE *fp);

/**
 * @brief Save base list to file path
 * @param list Pointer to base list
 * @param path File path
 * @return S_OK on success, E_FAIL on file error
 * @ingroup Storage
 */
P_HRESULT Save_BaseList(char *path);

/**
 * @brief Save target list to open file
 * @param fp Open FILE pointer for writing
 * @ingroup Storage
 */
void Save_TargetList_file(FILE *fp);

/**
 * @brief Save target list to file path
 * @param list Pointer to target list
 * @param path File path
 * @return S_OK on success, E_FAIL on file error
 * @ingroup Storage
 */
P_HRESULT Save_TargetList(char *path);

/**
 * @brief Save ammunition list to open file
 * @param fp Open FILE pointer for writing
 * @ingroup Storage
 */
void Save_Ammo_t_List_file(FILE *fp);

/**
 * @brief Save ammunition list to file path
 * @param list Pointer to ammunition list
 * @param path File path
 * @return S_OK on success, E_FAIL on file error
 * @ingroup Storage
 */
P_HRESULT Save_Ammo_t_List(char *path);

/** @} */



void PrintRangeHeader(FILE *fp, RangeType r);
P_HRESULT handle_save();

#endif /* STORAGE_H */