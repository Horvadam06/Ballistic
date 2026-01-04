#include "command\commands.h"
#include "helpers\error.h"
#include "helpers\debug.h"
#include "simulation\calculations.h"
#include "helpers\color.h"
#include "memory\load.h"
#include "helpers\helpers.h"
#include "os\terminal_info.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
        printf("┌─────────────────────────┬────────────┬───────────────────┐\n"
               "│description              │command     │aliases            │\n"
               "└─────────────────────────┴────────────┴───────────────────┘\n");

        print_header(NORMAL_SEPERATOR_SIZE,
            COLOR_TITLE" %-24s"COLOR_HINT" │ "COLOR_TITLE"%s"COLOR_HINT" │ "COLOR_TITLE"%s", "description", "command", "aliases");
        // add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        // printf(COLOR_TITLE" %-24s"COLOR_HINT" │ "COLOR_TITLE"%-10s"COLOR_HINT" │ "COLOR_TITLE"%-10s\n", "description", "command", "aliases");
        // add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    }
    for (size_t i = 0; i < command_count; i++) // goes trough the commands list, and print's out the aviable one
    {
        if (!(commands[i].case_mask & case_flags) || (commands[i].type == CMD_HELP))
            continue; // command not available in this mode

        printf(ANSI_GRAY " %-24s │" COLOR_TITLE "%-10s" ANSI_GRAY "  │", commands[i].description, commands[i].name);

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

Entry:
    clear_terminal();

Entry_no_clr:
    if (!(case_flags & CMD_C_NOCOMMENT))
    {
        printf("\n Targets menu\n");
        add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "list" ANSI_GRAY "]\n", "Display list");
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "add" ANSI_GRAY "]\n", "Add to list");
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "delete" ANSI_GRAY "]\n", "Delete from list");
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "back" ANSI_GRAY "]\n", "Step back");
        add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(ANSI_RESET);

        /*add_seperator_c(NULL, ANSI_YELLOW, "targets", '-', NORMAL_SEPERATOR_SIZE);
        printf("%-39s <[%s]>\n", "What would you like to do?", "back");
        printf("\t- %-30s [%s]\n", "show target list", "list");
        printf("\t- %-30s [%s]\n", "add target to list", "add");
        printf("\t- %-30s [%s]\n", "remove target from list", "delete");
        add_seperator_c(NULL, NULL, NULL, '-', NORMAL_SEPERATOR_SIZE);*/
    }

    Command_t cmd;
    while ((cmd = GetCommand(CMD_C_LOAD | CMD_C_MODAL, NULL)) != CMD_BACK)
    {
        switch (cmd)
        {
        case CMD_LIST:
            print_Target_List(target_list);
            return;
        case CMD_ADD:
        {
            clear_terminal();
            printf("\n Add target\n");
            add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
            printf(
                COLOR_INFO
                " Format:\n"
                "   I<id> L<label> CLA<lat> CLO<lon> CA<alt> A<area>\n"
                "\n"
                " Example:\n"
                "   I1 LAlpha CLA47.5 CLO19.0 CA120 A14.3\n");
            printf("Type 'back' to return to the previous menu.\n");
            add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);

        Entry_add_no_clr:
            char buffer[1024];

            int req = back_request(buffer, sizeof(buffer));
            if (req == 1)
                goto Entry;
            if (req == -1)
                goto Entry_add_no_clr;

            buffer[strcspn(buffer, "\r\n")] = 0;

            if (handle_target(buffer, target_list, NULL) == P_S_FALSE)
            {
                clear_terminal();
                print_colored(COLOR_ERROR, "Invalid input!\n", 1);
                goto Entry_no_clr;
            }

            print_colored(COLOR_SUCCESS, "List updated!\n", 1);
            goto Entry_add_no_clr;
        }
        case CMD_DELETE:
        {
            clear_terminal();
            printf("\n Delete target\n");
            add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
            printf(
                COLOR_INFO
                " Enter the ID of the target to delete.\n"
                " Type 'back' to return to the previous menu.\n");
            add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);

        Entry_delete_no_clr:

            char buffer[1024];
            int req_val = back_request(buffer, sizeof(buffer));
            if (req_val == 1)
            {
                goto Entry;
            }
            if (req_val == -1)
            {
                goto Entry_delete_no_clr;
            }

            buffer[strcspn(buffer, "\r\n")] = 0;
            unsigned id = (unsigned)strtoul(buffer, NULL, 10);

            Target *t = NULL;
            if (isalpha((unsigned char)buffer[0]) || !(t = FindTargetByID(target_list, id)))
            {
                clear_terminal();
                print_colored(COLOR_ERROR, "Invalid input!\n", 1);
                goto Entry_no_clr;
            }
            else
            {
                printf("\n Selected target:\n");
                printf("   ID: %u\n", t->id);
                printf("   Label: %s\n", t->label);
                printf("   Location: %.3f°, %.3f°\n", t->coord.latitude, t->coord.longitude);
                printf("   Altitude: %.2f m\n", t->coord.altitude);

                printf("\n Confirm deletion of this target? ");
                cmd_help_inline(NULL, CMD_C_YESNO, NULL);

                printf("\n");
                if (GetCommand(CMD_C_MODAL | CMD_C_YESNO, NULL) == CMD_NO)
                {
                    printf(COLOR_ERROR " Operation cancelled.\n");
                    goto Entry_delete_no_clr;
                }

                if (delete_Target(target_list, id))
                    printf(COLOR_SUCCESS "List updated!\n");
                else
                    printf(COLOR_ERROR "Delete failed!\n");
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
 * @brief Base management command handler
 *
 * Note: Structure is nearly identical to cmd_target() and cmd_ammo().
 * So in the future I consider refactoring these function's into a general helper function
 */
void cmd_base(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)running;
Entry:
    clear_terminal();
Entry_no_clr:
    if (!(case_flags & CMD_C_NOCOMMENT))
    {
        printf("\n Bases menu\n");
        add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "list" ANSI_GRAY "]\n", "Display list");
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "add" ANSI_GRAY "]\n", "Add to list");
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "delete" ANSI_GRAY "]\n", "Delete from list");
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "back" ANSI_GRAY "]\n", "Step back");
        add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(ANSI_RESET);

        /* add_seperator_c(ANSI_GRAY, ANSI_YELLOW, "bases", '-', NORMAL_SEPERATOR_SIZE);
         printf("%-39s <[%s]>\n", "What would you like to do?", "back");
         printf("\t- %-30s [%s]\n", "show base list", "list");
         printf("\t- %-30s [%s]\n", "add base to list", "add");
         printf("\t- %-30s [%s]\n", "remove base from list", "delete");
         add_seperator_c(NULL, NULL, NULL, '-', NORMAL_SEPERATOR_SIZE);*/
    }

    // waits for a valid command
    // on list, it exit's. In other cases it just returns, making it easier to input multiple parameters
    Command_t cmd;
    while ((cmd = GetCommand(CMD_C_LOAD | CMD_C_MODAL, NULL)) != CMD_BACK)
        switch (cmd)
        {
        case CMD_LIST:
            print_Base_List(base_list, S_DEBUG);
            return;
        case CMD_ADD:
        {
            clear_terminal();
            printf("\n Add base\n");
            add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
            printf(
                COLOR_INFO
                " Format:\n"
                "   I<id> L<label> CLA<lat> CLO<lon> CA<alt> AMS<:{id:count,...}>\n"
                "\n"
                " Example:\n"
                "   I1 LAlpha CLA47.5 CLO19.0 CA120 AMS:{2:50,5:10}\n");
            printf("Type 'back' to return to the previous menu.\n");
            add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);

        Entry_add_no_clr:

            char buffer[1024];
            int req_val = back_request(buffer, sizeof(buffer));
            if (req_val == 1)
            {
                goto Entry;
            }
            if (req_val == -1)
            {
                goto Entry_add_no_clr;
            }

            buffer[strcspn(buffer, "\r\n")] = 0;

            if (handle_base(buffer, base_list, NULL) == P_S_FALSE)
            {
                clear_terminal();
                print_colored(COLOR_ERROR, "Invalid input!\n", 1);
                goto Entry_no_clr;
            }
            else
            {
                print_colored(COLOR_SUCCESS, "List updated!\n", 1);
                goto Entry_add_no_clr;
            }
            break;
        }
        case CMD_DELETE:
        {
            clear_terminal();
            printf("\n Delete base\n");
            add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
            printf(
                COLOR_INFO
                " Enter the ID of the base to delete.\n"
                " Type 'back' to return to the previous menu.\n");
            add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);

        Entry_delete_no_clr:

            char buffer[1024];
            int req_val = back_request(buffer, sizeof(buffer));
            if (req_val == 1)
            {
                goto Entry;
            }
            if (req_val == -1)
            {
                goto Entry_delete_no_clr;
            }

            buffer[strcspn(buffer, "\r\n")] = 0;
            unsigned id = (unsigned)strtoul(buffer, NULL, 10);

            Base *b = NULL;
            if (isalpha((unsigned char)buffer[0]) || !(b = FindBaseByID(base_list, id)))
            {
                clear_terminal();
                print_colored(COLOR_ERROR, "Invalid input!\n", 1);
                goto Entry_no_clr;
            }
            else
            {
                printf("\n Selected base:\n");
                printf("   ID: %u\n", b->id);
                printf("   Label: %s\n", b->label);
                printf("   Location: %.3f°, %.3f°\n", b->coord.latitude, b->coord.longitude);
                printf("   Altitude: %.2f m\n", b->coord.altitude);

                printf("\n Confirm deletion of this base? ");
                cmd_help_inline(NULL, CMD_C_YESNO, NULL);

                printf("\n");
                if (GetCommand(CMD_C_MODAL | CMD_C_YESNO, NULL) == CMD_NO)
                {
                    printf(COLOR_ERROR " Operation cancelled.\n");
                    goto Entry_delete_no_clr;
                }

                if (delete_BaseById(base_list, id) != -1)
                    printf(COLOR_SUCCESS "List updated!\n");
                else
                    printf(COLOR_ERROR "Delete failed!\n");
                goto Entry_delete_no_clr;
            }
            break;
        }
        default:
            break;
        }
    cmd_clr_ter(NULL, CMD_C_ALL, NULL);
}
/**
 * @brief Base management command handler
 *
 * Note: Structure is nearly identical to cmd_target() and cmd_base().
 * So in the future I consider refactoring these function's into a general helper function
 */
void cmd_ammo(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)running;

Entry:
    clear_terminal();

Entry_no_clr:
    if (!(case_flags & CMD_C_NOCOMMENT))
    {

        printf("\n Ammo type menu\n");
        add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "list" ANSI_GRAY "]\n", "Display list");
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "add" ANSI_GRAY "]\n", "Add to list");
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "delete" ANSI_GRAY "]\n", "Delete from list");
        printf(ANSI_GRAY " %-20s : [" COLOR_TITLE "back" ANSI_GRAY "]\n", "Step back");
        add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(ANSI_RESET);

        /*add_seperator_c(NULL, ANSI_YELLOW, "ammo", '-', NORMAL_SEPERATOR_SIZE);
        printf("%-39s <[%s]>\n", "What would you like to do?", "back");
        printf("\t- %-30s [%s]\n", "show ammo type list", "list");
        printf("\t- %-30s [%s]\n", "add ammo type to list", "add");
        printf("\t- %-30s [%s]\n", "remove ammo type from list", "delete");
        add_seperator_c(NULL, NULL, NULL, '-', NORMAL_SEPERATOR_SIZE);*/
    }

    Command_t cmd;
    while ((cmd = GetCommand(CMD_C_LOAD | CMD_C_MODAL, NULL)) != CMD_BACK)
    {
        switch (cmd)
        {
        case CMD_LIST:
            print_Ammo_t_List(ammo_t_list);
            return;
        case CMD_ADD:
        {
            clear_terminal();
            add_seperator_c(NULL, ANSI_YELLOW, "add ammo", '-', NORMAL_SEPERATOR_SIZE);
            printf("Use the following structure for the input:\n"
                   "\t[I]   : id of ammo\n"
                   "\t[L] : label(name)\n"
                   "\t[W] : weight\n"
                   "\t[D] : diameter (caliber)\n"
                   "\t[BR] : blast radius\n"
                   "\t[BC] : ballistic coefficient\n");
            add_seperator_c(NULL, NULL, NULL, '-', NORMAL_SEPERATOR_SIZE);
        Entry_add_no_clr:

            char buffer[1024];

            int req = back_request(buffer, sizeof(buffer));
            if (req == 1)
                goto Entry;
            if (req == -1)
                goto Entry_add_no_clr;

            buffer[strcspn(buffer, "\r\n")] = 0;

            if (handle_ammo_t(buffer, ammo_t_list, NULL) == P_S_FALSE)
            {
                clear_terminal();
                print_colored(COLOR_ERROR, "Invalid input!\n", 1);
                goto Entry_no_clr;
            }

            print_colored(COLOR_SUCCESS, "List updated!\n", 1);
            goto Entry_add_no_clr;
        }
        case CMD_DELETE:
        {
            clear_terminal();
            printf("\n Delete ammo\n");
            add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
            printf(
                COLOR_INFO
                " Enter the ID of the ammo to delete.\n"
                " Type 'back' to return to the previous menu.\n");
            add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);

        Entry_delete_no_clr:

            char buffer[1024];
            int req_val = back_request(buffer, sizeof(buffer));
            if (req_val == 1)
            {
                goto Entry;
            }
            if (req_val == -1)
            {
                goto Entry_delete_no_clr;
            }

            buffer[strcspn(buffer, "\r\n")] = 0;
            unsigned id = (unsigned)strtoul(buffer, NULL, 10);

            Ammo_t *a = NULL;
            if (isalpha((unsigned char)buffer[0]) || !(a = FindAmmoByID(ammo_t_list, id)))
            {
                clear_terminal();
                print_colored(COLOR_ERROR, "Invalid input!\n", 1);
                goto Entry_no_clr;
            }
            else
            {
                printf("\n Selected ammo:\n");
                printf("   ID: %u\n", a->id);
                printf("   Label: %s\n", a->name);
                printf("   Mass: %f\n", a->weight);
                printf("   Bore: %f\n", a->diameter);
                printf("\n Confirm deletion of this target? ");
                cmd_help_inline(NULL, CMD_C_YESNO, NULL);

                printf("\n");
                if (GetCommand(CMD_C_MODAL | CMD_C_YESNO, NULL) == CMD_NO)
                {
                    printf(COLOR_ERROR " Operation cancelled.\n");
                    goto Entry_delete_no_clr;
                }

                if (delete_Ammo_t(ammo_t_list, id) != -1)
                    printf(COLOR_SUCCESS "List updated!\n");
                else
                    printf(COLOR_ERROR "Delete failed!\n");
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
    printf(ANSI_GRAY " %-10s : " COLOR_TITLE "Ballistic trajectory calculator\n", "Product");
    printf(ANSI_GRAY " %-10s : " COLOR_TITLE "Adam Jozsef Horvath\n", "Author");
    printf(ANSI_GRAY " %-10s : " COLOR_TITLE "7.3.0\n", "Version");
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
    printf(ANSI_GRAY " %-10s : " COLOR_TITLE "%d\n", "Terminal width", terminal_width());
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