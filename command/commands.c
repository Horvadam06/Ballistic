#include "command\commands.h"
#include "helpers\error.h"
#include "helpers\debug.h"
#include "simulation\calculations.h"
#include "helpers\color.h"
#include "memory\load.h"
#include "helpers\helpers.h"
#include "os\terminal_info.h"
#include "os\about_program.h"
#include "os\input_poll.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define HELP_DESCRIPTION_ALIGMENT 24

//========================================================================
// GLOBAL COMMAND TABLE
//========================================================================

// Command table defining all available commands in the system
// Format: {command_name, command_type, handler_function, description, context_flags}
// NOTE: Commands with NULL handlers are handled specially by GetCommand()
Command commands[] = {
    // Navigation and basic operations - these have NULL handlers as they're
    // handled directly in modal contexts (like cmd_base, cmd_target loops)

    CMD_INIT("cancel", CMD_CANCEL, NULL, "Cancel event", CMD_C_CANCEL),
    CMD_INIT("select", CMD_SELECT, NULL, "Select choice", CMD_C_SELECT),
    CMD_INIT("list", CMD_LIST, NULL, "List values", CMD_C_LOAD),
    CMD_INIT("add", CMD_ADD, NULL, "Add a value", CMD_C_LOAD),
    CMD_INIT("delete", CMD_DELETE, NULL, "Delete a value", CMD_C_LOAD),
    CMD_INIT("back", CMD_BACK, NULL, "Step back", CMD_C_LOAD | CMD_C_SELECT | CMD_C_BACK),
    CMD_INIT("next", CMD_NEXT, NULL, "Switch to next", CMD_C_SWITCH),
    CMD_INIT("previous", CMD_PREV, NULL, "Switch to previous", CMD_C_SWITCH),

    CMD_INIT("bases", CMD_BASE, cmd_base, "Work with bases", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_CALC),
    CMD_INIT("targets", CMD_TARGET, cmd_target, "Work with targets", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_CALC),
    CMD_INIT("ammo", CMD_AMMO, cmd_ammo, "Work with ammo", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_CALC),
    CMD_INIT("calculate", CMD_CALCULATE, cmd_calculate, "Calculate trajectory", CMD_C_GENERIC | CMD_C_DEFAULT),

    CMD_INIT("test", CMD_TEST, cmd_test, "Do a manual self-test", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM),
    CMD_INIT("clear", CMD_CLEAR_TERMINAL, cmd_clr_ter, "Clear the terminal", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM),
    CMD_INIT("config", CMD_CONFIG, cmd_config, "Modify config", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM),
    CMD_INIT("info", CMD_INFO, cmd_info, "Show program info", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM | CMD_C_STAYMODAL),
    CMD_INIT("save", CMD_SAVE, cmd_save, "Save changes", CMD_C_EXIT | CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM),
    CMD_INIT("exit", CMD_EXIT, cmd_exit, "Exit program", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM),
    CMD_INIT("debug", CMD_DEBUG, cmd_debug, "Debug info", CMD_C_DEBUG | CMD_C_STAYMODAL),

    CMD_INIT("help", CMD_HELP, cmd_help, "Show commands", CMD_C_ALL | CMD_C_STAYMODAL),

    CMD_INIT("colour", CMD_COLOUR, cmd_colour, "Coloured mode command", CMD_C_SETUP),
    CMD_INIT("mono", CMD_MONO, cmd_mono, "Monochrome mode command", CMD_C_SETUP),

    CMD_INIT("yes", CMD_YES, NULL, "Yes", CMD_C_YESNO),
    CMD_INIT("no", CMD_NO, NULL, "No", CMD_C_YESNO)};

static const CommandAlias aliases[] = {
    {"y", CMD_YES},
    {"n", CMD_NO},
    {"b", CMD_BACK},
    {"nxt", CMD_NEXT},
    {"prv", CMD_PREV},
    {"prev", CMD_PREV},
    {"bas", CMD_BASE},
    {"tgt", CMD_TARGET},
    {"am", CMD_AMMO},
    {"tst", CMD_TEST},
    {"conf", CMD_CONFIG},
    {"cnf", CMD_CONFIG},
    {"s", CMD_SAVE},
    {"d", CMD_DEBUG},
    {"c", CMD_CANCEL},
    {"clc", CMD_CALCULATE},
    {"calc", CMD_CALCULATE},
    {"sel", CMD_SELECT},
    {"e", CMD_EXIT},
    {"clr", CMD_CLEAR_TERMINAL},
    {"h", CMD_HELP},
    {"i", CMD_INFO},
    {"del", CMD_DELETE},
    {"ls", CMD_LIST},
};

/*
Calculates number of commands in the commands array
sizeof(commands) gives total bytes of the array
sizeof(commands[0]) gives bytes per element
Division yields element count - automatically updates when commands are added
*/
static size_t command_count = sizeof(commands) / sizeof(commands[0]);
static size_t alias_count = sizeof(aliases) / sizeof(aliases[0]);
static size_t command_key_count = 0;

static CommandKey *command_keys = NULL;

//========================================================================
// COMMAND HANDLER IMPLEMENTATIONS
//========================================================================

/**
 * @brief Exit program handler
 *
 * Prompts for confirmation before setting process state to S_END.
 * Handles edge case where running pointer might be NULL.
 *
 * Implementation details:
 * - Uses CMD_C_LOOP flag to stay in confirmation mode
 * - Only CMD_YESNO commands are allowed during confirmation
 * - Sets *running = S_END to signal main loop termination
 */
void cmd_exit(const char *args, Command_case case_flags, PROCESS *running)
{
    // needed for the compiler
    (void)args;
    (void)case_flags;

    printf("Are you sure you want to exit? ");
    cmd_help_inline(NULL, CMD_C_YESNO, NULL);
    printf("\n");
    if (GetCommand(CMD_C_MODAL | CMD_C_YESNO, NULL) == CMD_NO)
    {
        clear_terminal();
        return;
    }

    printf("Exiting...\n");
    // set's the process state, to S_END, which exit's the modal loop
    if (running)
        *running = S_END;
    else // error handling
    {
        printf("Running pointer error\n");
    }
}
/**
 * @brief Help command handler
 *
 * Filters commands based on case_flags to show only context-appropriate commands.
 * The help command itself is excluded from the displayed list.
 *
 * Note: add_seperator_c() is assumed to handle NULL title for closing separator
 */

void cmd_help(const char *args, Command_case case_flags, PROCESS *running)
{
    // needed for the compiler
    (void)args;
    (void)running;

    // formated list
    if (!(case_flags & CMD_C_NOCOMMENT))
    {
        add_header(NORMAL_SEPERATOR_SIZE,
                   COLOR_HINT,
                   COLOR_TITLE "%-*s" COLOR_HINT " |" COLOR_TITLE "%-10s" COLOR_HINT " |" COLOR_TITLE "%s",
                   HELP_DESCRIPTION_ALIGMENT, "description", "command", "aliases");
    }
    else
        add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    for (size_t i = 0; i < command_count; i++) // goes trough the commands list, and print's out the aviable one
    {
        if (!(commands[i].case_mask & case_flags) || (commands[i].type == CMD_HELP))
            continue; // command not available in this mode

        printf(ANSI_GRAY " %-*s │" COLOR_TITLE "%-10s" ANSI_GRAY " │", HELP_DESCRIPTION_ALIGMENT, commands[i].description, commands[i].name);

        /*change_colour(ANSI_GREEN);
        printf("    %-4s", commands[i].name);

        change_colour(ANSI_MAGENTA);
        printf("\t- %s ", commands[i].description);*/
        int alias_cnt = 0;

        for (size_t j = command_count; j < command_key_count; j++)
        {
            if (command_keys[j].cmd != &commands[i])
                continue;
            if (alias_cnt++ > 0)
                printf("/");
            printf(COLOR_TITLE "%s" ANSI_GRAY, command_keys[j].key);
        }

        printf(ANSI_RESET "\n");
    }
    add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
}

void cmd_help_inline(const char *args, Command_case case_flags, PROCESS *running)
{
    // needed for the compiler
    (void)args;
    (void)running;

    // formated list
    printf(ANSI_RESET "[");
    int is_first = 1;
    for (size_t i = 0; i < command_count; i++) // goes trough the commands list, and print's out the aviable one
    {
        if (!(commands[i].case_mask & case_flags) || (commands[i].type == CMD_HELP))
            continue; // command not available in this mode

        if (is_first)
            is_first = 0;
        else
            printf("/");

        printf(ANSI_GREEN);
        printf("%s", commands[i].name);

        printf(ANSI_RESET);
    }
    printf("]");
}

// Info display =========================================================

P_HRESULT base_info_print(ListManagerConfig *config, DisplayType display, void *entry_ptr, void *data)
{
    switch (display)
    {
    case LIST_MENU_ENTRY:
        printf("\n");
        add_header(NORMAL_SEPERATOR_SIZE, ANSI_GRAY, "%-*s " ANSI_GRAY "|" COLOR_RESET "%s", HELP_DESCRIPTION_ALIGMENT, "Bases menu", "command");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "list" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Display list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "add" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Add to list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "delete" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Delete from list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "back" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Step back");
        add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(ANSI_RESET);
        break;

    case LIST_MENU_ADD:

        // phrase state data pointer to state
        int state = (data) ? *((int *)data) : 0;

        if (!general_state_options(config->add_steps, config->add_steps_n, config->add_steps_prot_pos, state))
            return P_S_FALSE;

        break;

    case LIST_MENU_DELETE:
        printf("\n Delete base\n");
        add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(
            COLOR_INFO
            " Enter the ID of the base to delete.\n"
            " Type 'back' to return to the previous menu.\n");
        add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        break;

    case LIST_MENU_DEL_CONFIR:
    {
        Base *b = (Base *)entry_ptr;
        printf("\n Selected base:\n");
        printf("   ID: %u\n", b->id);
        printf("   Label: %s\n", b->label);
        printf("   Location: %.3f°, %.3f°\n", b->coord.latitude, b->coord.longitude);
        printf("   Altitude: %.2f m\n", b->coord.altitude);
        printf("\n");
        break;
    }
    default:
        Error_message("Missing display type!\n");
    }

    return P_S_OK;
}

P_HRESULT target_info_print(ListManagerConfig *config, DisplayType display, void *entry_ptr, void *data)
{
    switch (display)
    {
    case LIST_MENU_ENTRY:
        printf("\n");
        add_header(NORMAL_SEPERATOR_SIZE, ANSI_GRAY, "%-*s " ANSI_GRAY "|" COLOR_RESET "%s", HELP_DESCRIPTION_ALIGMENT, "Targets menu", "command");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "list" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Display list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "add" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Add to list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "delete" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Delete from list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "back" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Step back");
        add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(ANSI_RESET);
        break;

    case LIST_MENU_ADD:

        int state = (data) ? *((int *)data) : 0;
        const char *step[] = {
            "Add target wizard",
            "Add target",
            "Enter ID",
            "Enter label",
            "Enter position",
            "Enter altitude",
            "Enter area",
            "Confirm"};

        if (!general_state_options(step, SIZE_OF_STRING_ARRAY(step), 1, state))
            return P_S_FALSE;

        break;

    case LIST_MENU_DELETE:
        printf("\n Delete target\n");
        add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(
            COLOR_INFO
            " Enter the ID of the target to delete.\n"
            " Type 'back' to return to the previous menu.\n");
        add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        break;

    case LIST_MENU_DEL_CONFIR:
    {
        Target *t = (Target *)entry_ptr;
        printf("\n Selected target:\n");
        printf("   ID: %u\n", t->id);
        printf("   Label: %s\n", t->label);
        printf("   Location: %.3f°, %.3f°\n", t->coord.latitude, t->coord.longitude);
        printf("   Altitude: %.2f m\n", t->coord.altitude);
        printf("   Area: %.2f m²\n", t->area);
        printf("\n");
        break;
    }
    default:
        Error_message("Missing display type!\n");
    }
    return P_S_OK;
}

P_HRESULT ammo_info_print(ListManagerConfig *config, DisplayType display, void *entry_ptr, void *data)
{
    switch (display)
    {
    case LIST_MENU_ENTRY:
        printf("\n");
        add_header(NORMAL_SEPERATOR_SIZE, ANSI_GRAY, "%-*s " ANSI_GRAY "|" COLOR_RESET "%s", HELP_DESCRIPTION_ALIGMENT, "Ammo type menu", "command");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "list" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Display list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "add" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Add to list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "delete" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Delete from list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "back" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Step back");
        add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(ANSI_RESET);

        break;
    case LIST_MENU_ADD:
        int state = (data) ? *((int *)data) : 0;
        const char *step[] = {
            "Add ammo_t wizard",
            "Add ammo_t",
            "Enter ID",
            "Enter label",
            "Enter weight",
            "Enter diameter",
            "Enter blast radius",
            "Enter ballistic coefficient",
            "Confirm"};

        if (!general_state_options(step, SIZE_OF_STRING_ARRAY(step), 1, state))
            return P_S_FALSE;

        break;
    case LIST_MENU_DELETE:
        printf("\n Delete ammo\n");
        add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(
            COLOR_INFO
            " Enter the ID of the ammo to delete.\n"
            " Type 'back' to return to the previous menu.\n");
        add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        break;
    case LIST_MENU_DEL_CONFIR:
        if (!entry_ptr)
        {
            Error_message("Missing node ptr!\n");
            return P_S_FALSE;
        }
        Ammo_t *a = (Ammo_t *)entry_ptr;
        printf("\n Selected ammo:\n");
        printf("   ID: %u\n", a->id);
        printf("   Label: %s\n", a->name);
        printf("   Mass: %f\n", a->weight);
        printf("   Bore: %f\n", a->diameter);
        printf("\n");
        break;
    default:
        Error_message("Missing display type!\n");
    }

    return P_S_OK;
}

// Phrase add state to buffer ============================================

P_HRESULT base_string_state_token(char *buffer, size_t buffer_n, char *token, size_t token_n, int state)
{
    // plan
    // search for the required token with strchr, if it's in the 0th pos, it's a token for sure, if not
    // check if there is an empty space before it, if there is it's a token for sure
    // THIS CHECK CAN ONLY WORK IF I DON'T ALLOW SPACES IN FOR EXAMPLE LSOMETHING CLA, can only work with SOMETHING_CLA,
    // maybe rework the phraser, to convert the user input to automaticly add _ in the place of spaces

    // PSUEDO PLAN (or something)
    //   check the state validity
    //   check the input type validity
    //   search for the token id in the buffer
    //       if (exists)
    //           replace it, and offset everything by the pre calculated input length
    //       else
    //           add the id to it & slap it to the end, and let the handler take care of it
    //
    //

    const int STATE_CONFIRM = 6;
    // state validity check
    if (state > STATE_CONFIRM || state < 1)
        return P_S_FALSE;
    else if (state == STATE_CONFIRM)
    {
        return handle_base(buffer, base_list, NULL);
    }

    state--;
    char *id[] = {
        "I",          // id
        "L",          // label
        "CLA", "CLO", // coordinates (latitude, longitude)[degree]
        "CA",         // altitude [meter]
        "AMS"         // ammo stock
    };
    int failed = 1;
    char *token_srch = buffer;
    while (failed)
    {
        if (!token_srch)
            break;
        // check if it exists already in the buffer
        token_srch = strstr(token_srch, id[state]);
        if (token_srch == buffer)
        {
            failed = 0;
        }
        else
        {
            // check the space before the value
        }
    }

    switch (state)
    {
    case 0:
    {
        token_srch = strstr(buffer, "I");
        if (token_srch == buffer)
        {
        }

        break;
    }
    case 1:
    {
        token_srch = strstr(buffer, "L");
        if (token_srch == buffer)
        {
        }

        break;
    }
    case 2:
    {
        token_srch = strstr(buffer, "CLA");
        if (token_srch == buffer)
        {
        }

        break;
    }
    case 3:
    {
        token_srch = strstr(buffer, "CA");
        if (token_srch == buffer)
        {
        }

        break;
    }
    case 4:
    {
        token_srch = strstr(buffer, "AMS");
        if (token_srch == buffer)
        {
        }

        break;
    }

    default:
        return P_S_FALSE;
    }

    return P_S_FALSE;
}

P_HRESULT target_string_state_token(char *buffer, size_t buffer_n, char *token, size_t tokent_n, int state)
{
    (void)buffer_n;
    (void)state;
    (buffer)[0] = 's';
    return P_S_FALSE;
}

P_HRESULT ammo_t_string_state_token(char *buffer, size_t buffer_n, char *token, size_t token_n, int state)
{
    (void)buffer_n;
    (void)state;
    (buffer)[0] = 's';
    return P_S_FALSE;
}

/**
 * @brief Target management command handler
 *
 * Provides a sub-menu system for target operations. Uses CMD_C_LOOP to create
 * a modal context where only target-related commands are available.
 *
 * Flow:
 * 1. Clear screen and show menu (unless CMD_C_NOCOMMENT is set)
 * 2. Enter loop waiting for commands (list/add/delete/back)
 * 3. 'list' displays targets and returns to caller
 * 4. 'add' and 'delete' process input and remain in loop for multiple operations
 * 5. 'back' exits the modal context and returns to previous menu
 */
void cmd_target(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)running;
    ListManagerConfig target_config = {
        .name = "target",
        .list_ptr = target_list,
        .print_ = (P_HRESULT (*)(ListManagerConfig *, DisplayType, void *, void *))target_info_print,
        .print_list = (void (*)(void *))print_Target_List,
        .phrase_tokens = (P_HRESULT (*)(char *, size_t, char *, size_t, int))target_string_state_token,
        .find_by_id = (void *(*)(void *, unsigned))FindTargetByID,
        .delete_by_id = (int (*)(void *, unsigned))delete_Target};

    general_list_menu(case_flags, &target_config);
}
/**
 * @brief Base management command handler
 *
 * Note: Structure is nearly identical to cmd_target() and cmd_ammo().
 * So in the future I consider refactoring these function's into a general helper function
 */
void print_Base_List_wrapper(Base_List *list)
{
    print_Base_List(list, S_RUNNING);
}

void cmd_base(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)running;
    const char *steps[] = {
        "Add base wizard",
        "Add base",
        "Enter ID",
        "Enter label",
        "Enter position",
        "Enter altitude",
        "Enter ammo stock",
        "Confirm"};

    ListManagerConfig base_config = {
        .name = "base",
        .list_ptr = base_list,
        .print_ = (P_HRESULT (*)(ListManagerConfig *, DisplayType, void *, void *))base_info_print,
        .print_list = (void (*)(void *))print_Base_List_wrapper,
        .phrase_tokens = (P_HRESULT (*)(char *, size_t, char *, size_t, int))base_string_state_token,
        .add_steps = steps,
        .add_steps_n = SIZE_OF_STRING_ARRAY(steps),
        .add_steps_prot_pos = 1,
        .find_by_id = (void *(*)(void *, unsigned))FindBaseByID,
        .delete_by_id = (int (*)(void *, unsigned))delete_BaseById};

    general_list_menu(case_flags, &base_config);
}
/**
 * @brief ammo management command handler
 *
 * Note: Structure is nearly identical to cmd_target() and cmd_base().
 * So in the future I consider refactoring these function's into a general helper function
 */
void cmd_ammo(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)running;

    ListManagerConfig ammo_config = {
        .name = "ammo_t",
        .list_ptr = ammo_t_list,
        .print_ = (P_HRESULT (*)(ListManagerConfig *, DisplayType, void *, void *))ammo_info_print,
        .print_list = (void (*)(void *))print_Ammo_t_List,
        .phrase_tokens = (P_HRESULT (*)(char *, size_t, char *, size_t, int))ammo_t_string_state_token,
        .find_by_id = (void *(*)(void *, unsigned))FindAmmoByID,
        .delete_by_id = (int (*)(void *, unsigned))delete_Ammo_t};

    general_list_menu(case_flags, &ammo_config);
}

static int wrap_state(int state, int min, int max)
{
    int range = max - min + 1;
    return min + (state - (min - range)) % range;
    // min +, because we can't go lower than that
    //
}

static int parse_state_selector(const char *s, int *out_state)
{

    if (s[0] != 's' && s[0] != 'S')
        return 0;
    if (!isdigit((unsigned char)s[1]))
        return 0;

    char *end;
    long v = strtol(s + 1, &end, 10);

    if (*end != '\0')
        return 0;

    *out_state = (int)v;
    return 1;
}

/**
 * @brief general_list_menu_add helper
 * @return 0 Entry_no_clr |
 * @return 1 Entry
 *
 *
 */
static int general_list_menu_add(ListManagerConfig *config)
{
    // it's preaty whacky, should revise it later
    int state = 1;
    char handler_buffer[1024] = {'\0'};
Entry_add:
    clear_terminal();
Entry_add_after_clr:
    printf(CURSOR_HIDE);
    if (config->print_(config, LIST_MENU_ADD, NULL, &state) == P_S_FALSE)
    {
        printf(CURSOR_SHOW);
        return 0;
    }

Entry_add_no_clr:

    char buffer[1024];
    // write the input into buffer, and process if back request
    printf("> " CURSOR_SHOW);
    while (!input_available())
        ;
    char edg_trig_inp = input_getchar();

    Command_t add_cmd = CMD_INVALID;

    if (edg_trig_inp == '\t')
        add_cmd = CMD_NEXT;

    else
    {
        
        ungetc(edg_trig_inp, stdin);
        putchar(edg_trig_inp);
        fgets(buffer, sizeof(buffer), stdin);
        // remove traling carrig feed or new line
        buffer[strcspn(buffer, "\r\n")] = 0;
        add_cmd = ParseCommandFromBuffer(buffer, CMD_C_BACK | CMD_C_SWITCH | CMD_C_NOCOMMENT, NULL);
    }

    int valid_step_cnt = config->add_steps_n - config->add_steps_prot_pos - 1;

    switch (add_cmd)
    {
    case CMD_MODAL:
        goto Entry_add_no_clr;
        break;
    case CMD_BACK:
        return 1;
        break;
    case CMD_NEXT:
        state = wrap_state(++state, 1, valid_step_cnt);
        goto Entry_add;
        break;
    case CMD_PREV:
        state = wrap_state(--state, 1, valid_step_cnt);
        // state = (state + valid_step_cnt - 1) % (valid_step_cnt + ((state == 1) ? 2 : 0));
        goto Entry_add;
        break;

    default:
        if (parse_state_selector(buffer, &state))
        {
            clear_terminal();
            // 0 causes some issue
            state = clamp_with_warning(state, 1, valid_step_cnt);
            goto Entry_add_after_clr;
        }
        break;
    }

    /*
    int req = back_request(buffer, sizeof(buffer));
    if (req == 1)
        return 1;
    if (req == -1)
        goto Entry_add_no_clr;

    // remove traling carrig feed or new line
    buffer[strcspn(buffer, "\r\n")] = 0;

    // remove when proper command handling
    if (!isalpha((unsigned char)buffer[0]))
    {
        state = (int)strtof(buffer, NULL);
        goto Entry_add;
    }*/

    // handle input error
    // option 1.
    // make it so it's somewhat self contained, it return if the token was correct with S_OK maybe, if no S_FALSE
    // probably a stand alone enum state machine would be the best, which returns the process state (finished, running, etc)
    // Always return's to Entry_add, so the command handling is already processed. If it signals finished,
    // only then continue with the process
    // ammo and target should be the easiest, bases ammo stock is the problem
    // Biggest issues:a very big chance of memory leak
    //
    // option 2.
    // make it fully self contained
    // pros: memory safe
    // cons : duplicated code for command handling, maybe can be avoided if the command managment is modified to be self contained

    // self contained command handling parameters, back, help, and that's it. Maybe can be avoided using buffer command handler???

    // something is needed, which ectracts the tyoe and the data from the token, and checks it's validity,
    //  if it's valid, then it writes it to the temp data type, on exit, or succes, it free's up all of the allocated memory, then calls the appripiate call function
    // basicly a per token handler
    //
    // the cleanest would be a self contained function, because then it won't pollute the command logic further,
    // with backend stuff, so should be at load.c or storage.c, the only problem would be, that it would cause the "gui" to freeze, or maybe just move the ui displaying to there
    // basicly it should be the same as the deticated handler, the only difference would be, that instead of the tokenizer stepping on the buffer, it would only phrase
    // one token
    //
    // this is a self contained loop, maybe create an internal buffer, which get's modified in the
    //"handler", it would string together the input data, and when finished pass it to the "real" handler??
    // the data can be raw, so no id tokens are needed, because they type would depend on the state
    //

    // maybe pass the struct to the function too, so it can display the value set, and not just rawdogging the value's by head
    // handler should be seperate, later I need to add it back
    // config->handler(buffer, config->list_ptr, NULL) == P_S_FALSE
    // internal state handling or some fuck
    if (config->phrase_tokens(
            handler_buffer, SIZE_OF_CHAR_ARRAY(handler_buffer),
            buffer, SIZE_OF_CHAR_ARRAY(buffer),
            state))
    {
        clear_terminal();
        print_colored(COLOR_ERROR, "Invalid input!\n", 1);
        return 0;
    }

    print_colored(COLOR_SUCCESS, "List updated!\n", 1);
    goto Entry_add_no_clr;

    return 0;
}

void general_list_menu(Command_case case_flags, ListManagerConfig *config)
{
    // function parameters
    //   -case_flags
    //
    //   -display function display data by call case display_base(Command_t cmd_t) this way it's static only,
    //    could make a unique enum for the display options
    //
    //   -list_print fp
    //
    //   -add_handler fp
    //
    //   -search_id fp
    //
    //   -delete_handler fp

    // 1. entry text
    // 3. add parameters
    // 4. add handler
    // 5. delete parameters

    // 2. list_print pointer
    // 6. find by id
    // 7. delete handler

Entry:
    clear_terminal();
Entry_no_clr:
    if (!(case_flags & CMD_C_NOCOMMENT))
    {
        if (!config->print_)
        {
            Error_message("Missing print_ parameter for %s!\n", config->name);
            return;
        }
        config->print_(config, LIST_MENU_ENTRY, NULL, NULL);
    }
    Command_t cmd;

    while ((cmd = GetCommand(CMD_C_LOAD | CMD_C_MODAL, NULL)) != CMD_BACK)
    {
        switch (cmd)
        {
        case CMD_LIST:
            config->print_list(config->list_ptr);
            return;
        case CMD_ADD:
        {
            if (general_list_menu_add(config))
                goto Entry;
            else
                goto Entry_no_clr;
        }
        case CMD_DELETE:
        {
            clear_terminal();
            config->print_(config, LIST_MENU_DELETE, NULL, NULL);

        Entry_delete_no_clr:

            char buffer[1024];
            // write the input into buffer, and process if back request
            int req_val = back_request(buffer, sizeof(buffer));
            if (req_val == 1)
                goto Entry;
            else if (req_val == -1)
                goto Entry_delete_no_clr;

            // remove trailing carrige return or new line
            buffer[strcspn(buffer, "\r\n")] = 0;
            // extract id
            unsigned id = (unsigned)strtoul(buffer, NULL, 10);

            // handle id
            void *node = NULL;
            if (isalpha((unsigned char)buffer[0]) || !(node = config->find_by_id(config->list_ptr, id)))
            {
                clear_terminal();
                print_colored(COLOR_ERROR, "Invalid input!\n", 1);
                goto Entry_no_clr;
            }
            else
            {
                config->print_(config, LIST_MENU_DEL_CONFIR, node, NULL);

                // confirm deletion
                printf("\n Confirm deletion? ");
                cmd_help_inline(NULL, CMD_C_YESNO, NULL);

                printf("\n");
                if (GetCommand(CMD_C_MODAL | CMD_C_YESNO, NULL) == CMD_NO)
                {
                    printf(COLOR_ERROR " Operation cancelled.\n" COLOR_RESET);
                    goto Entry_delete_no_clr;
                }

                // deletion error handling
                if (config->delete_by_id(config->list_ptr, id) != -1)
                    printf(COLOR_SUCCESS " List updated!\n");
                else
                    Error_message(" Delete failed!\n");
                printf(ANSI_RESET);
                goto Entry_delete_no_clr;
            }
        }
        default:
            break;
        }
    }
    cmd_clr_ter(NULL, CMD_C_ALL, NULL);
}

/**
 * @brief display state machine
 * @param array state string array
 * @param n array size
 * @param protected_pos choose the list start, -1 for no hidden string
 * @param step Pointer to program state
 * @ingroup CommandHandling
 */
int general_state_options(const char *const array[], int n, int protected_pos, int step)
{
    if (protected_pos < -1)
        protected_pos = -1;
    int start_i = protected_pos + 1;
    int adjusted_step = start_i + step - 1;
    if (adjusted_step >= n)
    {
        Error_message("For the array: 0x%p, the given step: %d, exceeds the array's limit: %d!\n", array, adjusted_step, n);
        return 0;
    }
    else if (adjusted_step <= protected_pos)
    {
        Error_message("For the array: 0x%p, the value: %d, is protected!\n", array, step);
        return 0;
    }

    if (protected_pos >= 0)
        add_header(NORMAL_SEPERATOR_SIZE, ANSI_GRAY, "%s", array[0]);

    // add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    for (int i = start_i; i < n; i++)
        printf(COLOR_RESET " %d.%s %s\n",
               i - start_i + 1, (i == adjusted_step) ? "" : COLOR_HINT, array[i]);
    add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);

    if (protected_pos >= 1)
        printf(COLOR_HINT "%s (%d/%d) : " COLOR_RESET "%s\n",
               array[1], step, n - protected_pos - 1, array[adjusted_step]);
    // add_header(NORMAL_SEPERATOR_SIZE, ANSI_GRAY, COLOR_HINT "%s (%d/%d) | " COLOR_RESET "%s",
    //             array[1], step, n - protected_pos - 1, array[adjusted_step]);

    return 1;
}
/**
 * @brief Test command handler
 *
 * Start's a test sequence in S_DEBUG mode, which performs tests on the lists, it was mainly used for the development process.
 * In the current state it provides a feedback for the user, or for further developments.
 */
void cmd_test(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)case_flags;
    (void)running;
    run_pre_test(S_DEBUG);
}
/**
 * @brief Colour mode command handler (Coloured)
 *
 * In the current state it's only used for chosing desired colour mode at config
 */
void cmd_colour(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)case_flags;
    (void)running;
    set_color_mode(CM_COLOUR);
}
/**
 * @brief Color mode command handler (Monochrome)
 *
 * In the current state it's only used for chosing desired colour mode at config
 */
void cmd_mono(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)case_flags;
    (void)running;
    set_color_mode(CM_MONO);
}
/**
 * @brief Clear terminal command handler
 *
 * It calls the helper clear_terminal(), which clear's the terminal if the ANSI escape code's are enabled
 * and choosed by the user at config.
 */
void cmd_clr_ter(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)case_flags;
    (void)running;
    clear_terminal();
}
/**
 * @brief Run config command handler
 *
 * It calls the function recreate_config(), which does the config rebuild process.
 */
void cmd_config(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)case_flags;
    (void)running;
    recreate_config("config.txt", S_DEBUG);
}
/**
 * @brief Calculation command handler
 *
 * Main interface for the calculation setup. Provides access to
 * parameter selection and entity management (bases/targets/ammo).
 *
 * Important: The goto EntryPoint pattern is used to return to the main
 * calculation menu after each operation.
 */
void cmd_calculate(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)running;
    cmd_clr_ter(NULL, CMD_C_ALL, NULL);
EntryPoint:
    Command_t cmd;
    if (!(case_flags & CMD_C_NOCOMMENT))
    {
        printf("\n Calculation menu\n");
        add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "select" ANSI_GRAY "]\n", "Calculation select");
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "bases" ANSI_GRAY "]\n", "Base menu");
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "targets" ANSI_GRAY "]\n", "Target menu");
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "ammo" ANSI_GRAY "]\n", "Ammo type menu");
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "back" ANSI_GRAY "]\n", "Step back");
        add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(ANSI_RESET);

        /*
                add_seperator_c(NULL, ANSI_YELLOW, "calculate", '-', NORMAL_SEPERATOR_SIZE);
                printf("%-39s <[%s]>\n", "What would you like to do?", "back");
                printf("\t- %-30s [%s]\n", "select parameters", "select");
                printf("\t- %-30s [%s]\n", "work with bases", "bases");
                printf("\t- %-30s [%s]\n", "work with target", "target");
                printf("\t- %-30s [%s]\n", "work with ammo", "ammo");
                add_seperator_c(NULL, NULL, NULL, '-', NORMAL_SEPERATOR_SIZE);*/
    }
    // waits for a valid command
    // on list, it exit's. In other cases it just returns, making it easier to make multiple calculations
    while ((cmd = GetCommand(CMD_C_MODAL | CMD_C_CALC | CMD_C_SELECT, NULL)) != CMD_BACK)
    {
        cmd_clr_ter(NULL, CMD_C_ALL, NULL);
        switch (cmd)
        {
        case CMD_SELECT:
            printf("\n Calculation\n");
            add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
            printf(ANSI_GRAY);
            printf(" Retry dumb fuck!\n");
            add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
            printf(ANSI_RESET);

            while (HandleParameterSelection() == 0)
                ;
            break;

        default:
            break;
        }
        goto EntryPoint;
    }
    cmd_clr_ter(NULL, CMD_C_ALL, NULL);
}
/**
 * @brief Save command handler
 *
 * A manaul handler for the save process, at exit the user gets a suggestion for saving automatically.
 */
void cmd_save(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)running;
    if (case_flags & CMD_C_NOHANDLE)
        return;
    if (!(case_flags & CMD_C_NOCOMMENT))
    {
        printf(" Are you sure, you want to save? ");
        cmd_help_inline(NULL, CMD_C_YESNO, NULL);
    }
    if (GetCommand(CMD_C_MODAL | CMD_C_YESNO, NULL) == CMD_NO)
        return;
    if (handle_save() == P_S_FALSE)
        printf(" Atleast on save has failed!\n");
}

/**
 * @brief info command handler
 *
 * Display the info about the program and the program author.
 */
void cmd_info(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)running;
    if (case_flags & CMD_C_NOCOMMENT)
        return;
    printf("\n");
    add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    printf(ANSI_GRAY " %-10s : " COLOR_TITLE P_NAME "\n", "Product");
    printf(ANSI_GRAY " %-10s : " COLOR_TITLE P_AUTHOR "\n", "Author");
    printf(ANSI_GRAY " %-10s : " COLOR_TITLE P_VERSION "\n", "Version");
    add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    printf(ANSI_RESET);
    printf("\n");
}

void cmd_debug(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)running;
    if (case_flags & CMD_C_NOCOMMENT)
        return;
    clear_terminal();
    printf(ANSI_GRAY ASCI_ANGEL ANSI_RESET);
    printf("\n");
    add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    int x, y;
    terminal_size(&x, &y);
    printf(ANSI_GRAY " %-10s : " COLOR_TITLE "%d\n", "Terminal width", x);
    printf(ANSI_GRAY " %-10s : " COLOR_TITLE "%d\n", "Terminal height", y);
    add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    printf(ANSI_RESET);
}
//========================================================================
// COMMAND PROCESSING CORE
//========================================================================

static int find_command_nontoken(const char *input, const char **cmd_start, size_t *cmd_len, const char **args)
{
    const char *p = input;

    while (*p == ' ' || *p == '\t')
        p++;

    if (*p == '\0')
        return 0;

    *cmd_start = p;
    *cmd_len = 0;

    while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
    {
        p++;
        (*cmd_len)++;
    }

    *args = (*p) ? skip_spaces(p) : NULL;
    return 1;
}

// similiar to find_command_nontoken, but can work in tokened, and non token mode
static int parse_command(char *input, Command_case flags, const char **cmd, size_t *cmd_len, const char **args)
{
    (void)flags;
    input[strcspn(input, "\n")] = '\0';

    // remove trailing newline
    if (!find_command_nontoken(input, cmd, cmd_len, args))
        return 0;

    if (!*args || **args == '\0')
        *args = NULL;

    return 1;
}

static int register_key(const char *key, Command *cmd)
{
    // Check for duplicates first
    for (size_t i = 0; i < command_key_count; i++)
    {
        if (command_keys[i].key && strcmp(command_keys[i].key, key) == 0)
        {
            fprintf(stderr, "Duplicate command key detected: '%s' used by commands '%s' and '%s'\n",
                    key, command_keys[i].cmd->name, cmd->name);
            return -1;
        }
    }

    // Add key to next available slot
    command_keys[command_key_count++] = (CommandKey){
        .key = key,
        .len = strlen(key),
        .cmd = cmd};

    return 0;
}

PROCESS build_command_keys()
{
    // Allocate maximum possible slots (commands + aliases)
    size_t max_keys = command_count + alias_count;
    command_keys = calloc(max_keys, sizeof(CommandKey));
    if (!command_keys)
    {
        fprintf(stderr, "Out of memory building command keys\n");
        return S_TERMINATE;
    }

    command_key_count = 0; // will increment inside register_key

    // Register primary commands
    for (size_t i = 0; i < command_count; i++)
    {
        if (register_key(commands[i].name, &commands[i]) < 0)
            return S_TERMINATE; // duplicate found, stop
    }

    // Register aliases
    for (size_t i = 0; i < alias_count; i++)
    {
        Command *cmd = NULL;
        // find command with matching type
        for (size_t j = 0; j < command_count; j++)
        {
            if (commands[j].type == aliases[i].target)
            {
                cmd = &commands[j];
                break;
            }
        }
        if (!cmd)
        {
            fprintf(stderr, "Alias '%s' points to unknown command type %d\n",
                    aliases[i].key, aliases[i].target);
            continue;
        }

        if (register_key(aliases[i].key, cmd) < 0)
            return S_TERMINATE;
    }

    return S_RUNNING;
}

static Command *lookup_command(const char *cmd, size_t len, Command_case flags)
{
    for (size_t i = 0; i < command_key_count; i++)
    {
        CommandKey *k = &command_keys[i];

        if (k->len != len)
            continue;

        if (memcmp(k->key, cmd, len) != 0)
            continue;

        if (k->cmd->case_mask & flags)
            return k->cmd;

        // error handling
        if (!(flags & CMD_C_NOCOMMENT))
            printf("Invalid command: [" ANSI_GREEN "%.*s" ANSI_RESET "]\n", (int)len, cmd);
        return NULL;
    }
    // error handling
    if (!(flags & CMD_C_NOCOMMENT))
        printf("Unknown command: [" ANSI_GREEN "%.*s" ANSI_RESET "]\n", (int)len, cmd);
    return NULL;
}

// handle the command thing, calling the ptr, returning the cmd etc.
static Command_t resolve_command(const char *cmd, size_t cmd_len, const char *args, Command_case flags, PROCESS *running)
{
    Command *c = lookup_command(cmd, cmd_len, flags);
    if (!c)
        for (size_t i = 0; i < command_count; i++)
        {
            if (commands[i].type != CMD_HELP) // seraches for the help flag,
                continue;                     // to print out it's current name (only for keeping it dynamic)
            if (!(flags & CMD_C_NOCOMMENT) &&
                (flags & commands[i].case_mask))
                printf("If you need help, use the command [" ANSI_GREEN "%s" ANSI_RESET "]\n", commands[i].name);

            return CMD_INVALID;
        }

    if (c->handler && !(flags & CMD_C_NOHANDLE))
        c->handler(args, flags, running);

    if (c->case_mask && (c->case_mask & CMD_C_STAYMODAL))
        return CMD_MODAL;

    return c->type;
}

void free_command_keys(void)
{
    free(command_keys);
    command_keys = NULL;
}

/**
 * @brief Get command from user with modal context
 *
 * Complex state machine handling multiple modes:
 * - CMD_C_LOOP: Continue prompting until valid command
 * - CMD_C_NOHANDLE: Skip command handler execution (for testing/pre-parsing)
 * - CMD_C_NOCOMMENT: Suppress error/help messages
 *
 * Input processing:
 * 1. Reads up to 127 characters (leaving room for null terminator)
 * 2. Normalizes to lowercase for case-insensitive matching
 * 3. Splits into command and arguments using whitespace delimiters
 *
 * Security note: fgets() is used with size limit to prevent buffer overflow
 */
Command_t GetCommand(Command_case case_flags, PROCESS *running)
{
    char input[128];
    Command_t command = CMD_INVALID;

    // if case_flags CMD_C_LOOP has been set, than the function run's till a valid input
    while (command == CMD_INVALID || command == CMD_MODAL)
    {
        printf("> ");
        if (!fgets(input, (sizeof(input) > 128) ? 128 : sizeof(input), stdin))
            continue;

        NormalizeInput(input);

        const char *cmd = NULL;
        size_t cmd_len = 0;
        const char *args = NULL;

        if (!parse_command(input, case_flags, &cmd, &cmd_len, &args))
            continue;

        command = resolve_command(cmd, cmd_len, args, case_flags, running);

        if (!(case_flags & CMD_C_MODAL))
            break;
    }
    return command;
}
/**
 * @brief Parse command from buffer (non-interactive)
 *
 * Batch-processing version of GetCommand(). Useful for:
 * - Reading commands from script files
 * - Processing queued commands
 * - Unit testing command handlers
 *
 * Note: Modifies input buffer in-place via NormalizeInput()
 */
Command_t ParseCommandFromBuffer(char *input, Command_case case_flags, PROCESS *running)
{
    NormalizeInput(input);

    const char *cmd = NULL;
    size_t cmd_len = 0;
    const char *args = NULL;

    if (!parse_command(input, case_flags, &cmd, &cmd_len, &args))
        return CMD_INVALID;

    return resolve_command(cmd, cmd_len, args, case_flags, running);
}

// return values: 1 if CMD_BACK | -1 if not CMD_BACK, but valid | 0 if none
int back_request(char input[], int length)
{
    printf("> ");
    if (!fgets(input, length, stdin))
        return -1;

    input[strcspn(input, "\r\n")] = 0; // remove newline

    // Check if input is a command (starts with letter)
    if (isalpha((unsigned char)input[0]))
    {
        Command_t cmd = ParseCommandFromBuffer(input, CMD_C_BACK | CMD_C_NOCOMMENT, NULL);
        if (cmd == CMD_BACK)
            return 1;
        if (cmd != CMD_INVALID)
            return -1;
    }

    return 0;
}