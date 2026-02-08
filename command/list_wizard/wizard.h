
#ifndef COMMAND_LIST_WIZ_H
#define COMMAND_LIST_WIZ_H

#include "helpers\error.h"
#include "memory\storage.h"
#include "command\commands.h"

#include <stdint.h>
#include <stdbool.h>


//==================================================
// Type drafts
//==================================================

typedef struct
{
    unsigned id;
    bool has_id;

    char *label;
    bool has_label;

    Coordinate coordinate;
    bool has_coord;
    bool has_altitude;

    Ammo_Stock_List *ammo;
    bool has_ammo;
} Base_Draft;


/* ============================================================
 * Generic List / Wizard Framework
 * ============================================================ */

typedef enum
{
    LIST_MENU_ENTRY,
    LIST_MENU_ADD,
    LIST_MENU_DELETE,
    LIST_MENU_DEL_CONFIR
} DisplayType;

// configuration structure for list manipulation function
typedef struct ListManagerConfig_
{
    const char *name;
    void *list_ptr;

    // Display functions
    P_HRESULT (*print_)(struct ListManagerConfig_ *config, DisplayType display, void *entry_ptr, void *data);
    void (*print_list)(void *list);

    // Input handling
    P_HRESULT (*phrase_tokens)(char *buffer, size_t buffer_n, char *token, size_t token_n, int state);
    void (*on_add)();

    // Search/delete
    void *(*find_by_id)(void *list, unsigned id);
    int (*delete_by_id)(void *list, unsigned id);

} ListManagerConfig;

void base_add_wizard(ListManagerConfig *cfg);

void general_list_menu(Command_case case_flags, ListManagerConfig *config);

P_HRESULT base_info_print(ListManagerConfig *config, DisplayType display, void *entry_ptr, void *data);
P_HRESULT target_info_print(ListManagerConfig *config, DisplayType display, void *entry_ptr, void *data);
P_HRESULT ammo_info_print(ListManagerConfig *config, DisplayType display, void *entry_ptr, void *data);

#endif