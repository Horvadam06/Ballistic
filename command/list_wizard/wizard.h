
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

typedef enum {
    BASE_ERR_NONE       = 0,
    BASE_ERR_ID         = 1 << 0,
    BASE_ERR_LABEL      = 1 << 1,
    BASE_ERR_COORD      = 1 << 2,
    BASE_ERR_ALTITUDE   = 1 << 3,
    BASE_ERR_AMMO       = 1 << 4
} BaseDraftError;

#define WIZARD_MSG_MAX 512

typedef struct
{
    int state;
    uint32_t error_mask;
    char ui_message[WIZARD_MSG_MAX];
    bool has_message;

    unsigned id;
    bool has_id;

    char *label;
    bool has_label;

    Coordinate coordinate;
    bool has_coord;
    bool has_altitude;

    Ammo_Stock_List *ammo_stock;
    bool has_ammo_stock;
} Base_Draft;

typedef enum
{
    WIZARD_CONFIRM,
    WIZARD_CANCEL,
    WIZARD_CONTINUE
} WizardAction;

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

int parse_state_selector(const char *s, int *out_state);

P_HRESULT base_info_print(ListManagerConfig *config, DisplayType display, void *entry_ptr, void *data);
P_HRESULT target_info_print(ListManagerConfig *config, DisplayType display, void *entry_ptr, void *data);
P_HRESULT ammo_info_print(ListManagerConfig *config, DisplayType display, void *entry_ptr, void *data);

#endif