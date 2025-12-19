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

//========================================================================
// GLOBAL COMMAND TABLE
//========================================================================

// Command table defining all available commands in the system
// Format: {command_name, command_type, handler_function, description, context_flags}
// NOTE: Commands with NULL handlers are handled specially by GetCommand()
Command commands[] = {
    // Navigation and basic operations - these have NULL handlers as they're
    // handled directly in modal contexts (like cmd_base, cmd_target loops)
    CMD_INIT("back", CMD_BACK, NULL, "Step back", CMD_C_LOAD | CMD_C_SELECT | CMD_C_BACK),
    CMD_INIT("cancel", CMD_CANCEL, NULL, "Cancel event", CMD_C_CANCEL),
    CMD_INIT("select", CMD_SELECT, NULL, "Select choice", CMD_C_SELECT),
    CMD_INIT("list", CMD_LIST, NULL, "List values", CMD_C_LOAD),
    CMD_INIT("add", CMD_ADD, NULL, "Add a value", CMD_C_LOAD),
    CMD_INIT("delete", CMD_DELETE, NULL, "Delete a value", CMD_C_LOAD),

    CMD_INIT("bases", CMD_BASE, cmd_base, "Work with bases", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_CALC),
    CMD_INIT("targets", CMD_TARGET, cmd_target, "Work with targets", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_CALC),
    CMD_INIT("ammo", CMD_AMMO, cmd_ammo, "Work with ammo", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_CALC),
    CMD_INIT("calc", CMD_CALCULATE, cmd_calculate, "Calculate trajectory", CMD_C_GENERIC | CMD_C_DEFAULT),

    CMD_INIT("test", CMD_TEST, cmd_test, "Do a manual self-test", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM),
    CMD_INIT("clear", CMD_CLEAR_TERMINAL, cmd_clr_ter, "Clear the terminal", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM),
    CMD_INIT("config", CMD_CONFIG, cmd_config, "Modify config", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM),
    CMD_INIT("info", CMD_INFO, cmd_info, "Show program info", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM),
    CMD_INIT("save", CMD_SAVE, cmd_save, "Save changes", CMD_C_EXIT | CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM),
    CMD_INIT("exit", CMD_EXIT, cmd_exit, "Exit program", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM),
    CMD_INIT("debug", CMD_DEBUG, cmd_debug, "Debug info", CMD_C_DEBUG),

    CMD_INIT("help", CMD_HELP, cmd_help, "Show commands", CMD_C_ALL),

    CMD_INIT("colour", CMD_COLOUR, cmd_colour, "Coloured mode command", CMD_C_SETUP),
    CMD_INIT("mono", CMD_MONO, cmd_mono, "Monochrome mode command", CMD_C_SETUP),

    CMD_INIT("yes", CMD_YES, NULL, "Yes", CMD_C_YESNO),
    CMD_INIT("no", CMD_NO, NULL, "No", CMD_C_YESNO)};

/*
Calculates number of commands in the commands array
sizeof(commands) gives total bytes of the array
sizeof(commands[0]) gives bytes per element
Division yields element count - automatically updates when commands are added
*/
int command_count = sizeof(commands) / sizeof(commands[0]);

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
    if (GetCommand(CMD_C_LOOP | CMD_C_YESNO, NULL) == CMD_NO)
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
    add_seperator_c(NULL, ANSI_YELLOW, "command list", '-', NORMAL_SEPERATOR_SIZE);
    for (int i = 0; i < command_count; i++) // goes trough the commands list, and print's out the aviable one
    {
        if (!(commands[i].case_mask & case_flags) || (commands[i].type == CMD_HELP))
            continue; // command not available in this mode

        change_colour(ANSI_GREEN);
        printf("    %-4s", commands[i].name);

        change_colour(ANSI_MAGENTA);
        printf("\t- %s\n", commands[i].description);

        change_colour(ANSI_RESET);
    }
    add_seperator_c(NULL, NULL, NULL, '-', NORMAL_SEPERATOR_SIZE);
}

void cmd_help_inline(const char *args, Command_case case_flags, PROCESS *running)
{
    // needed for the compiler
    (void)args;
    (void)running;

    // formated list
    printf(ANSI_RESET "[");
    int is_first = 1;
    for (int i = 0; i < command_count; i++) // goes trough the commands list, and print's out the aviable one
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
    while ((cmd = GetCommand(CMD_C_LOAD | CMD_C_LOOP, NULL)) != CMD_BACK)
    {
        switch (cmd)
        {
        case CMD_LIST:
            print_Target_List(target_list);
            return;
        case CMD_ADD:
        {
            clear_terminal();
            add_seperator_c(NULL, ANSI_YELLOW, "add target", '-', NORMAL_SEPERATOR_SIZE);
            printf("Use the following structure for the input:\n"
                   "\t[I]   : id of target\n"
                   "\t[L] : label(name)\n"
                   "\t[CLA] : latitude(degrees)\n"
                   "\t[CLO] : longitude(degrees)\n"
                   "\t[CA] : altitude(meters)\n"
                   "\t[A] : area (km^2)\n");
            add_seperator_c(NULL, NULL, NULL, '-', NORMAL_SEPERATOR_SIZE);

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
            add_seperator_c(NULL, ANSI_YELLOW, "delete target", '-', NORMAL_SEPERATOR_SIZE);
            printf("Use the following structure for the input:\n");
            printf("\t[I]   : id of target\n");
            add_seperator_c(NULL, NULL, NULL, '-', NORMAL_SEPERATOR_SIZE);
        Entry_delete_no_clr:

            char buffer[1024];
            int req = back_request(buffer, sizeof(buffer));
            if (req == 1)
                goto Entry;
            if (req == -1)
                goto Entry_delete_no_clr;

            buffer[strcspn(buffer, "\r\n")] = 0;
            unsigned id = (unsigned)strtoul(buffer, NULL, 10);

            if (delete_Target(target_list, id) == -1)
            {
                clear_terminal();
                print_colored(COLOR_ERROR, "Invalid ID!\n", 1);
                goto Entry_no_clr;
            }

            print_colored(COLOR_SUCCESS, "List updated!\n", 1);
            goto Entry_delete_no_clr;
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
    while ((cmd = GetCommand(CMD_C_LOAD | CMD_C_LOOP, NULL)) != CMD_BACK)
        switch (cmd)
        {
        case CMD_LIST:
            print_Base_List(base_list, S_DEBUG);
            return;
        case CMD_ADD:
        {
            clear_terminal();
            add_seperator_c(NULL, ANSI_YELLOW, "add base", '-', NORMAL_SEPERATOR_SIZE);
            printf("Use the following structure for the input:\n"
                   "\t[I]   : id of base\n"
                   "\t[L] : label(name)\n"
                   "\t[CLA] : latitude(degrees)\n"
                   "\t[CLO] : longitude(degrees)\n"
                   "\t[CA] : altitude(meters)\n"
                   "\t[AMS] : ammo stock map {ammo_id:count, ...}\n");
            add_seperator_c(NULL, NULL, NULL, '-', NORMAL_SEPERATOR_SIZE);

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
            add_seperator_c(NULL, ANSI_YELLOW, "delete base", '-', NORMAL_SEPERATOR_SIZE);
            printf("Use the following structure for the input:\n");
            printf("\t[I]   : id of base\n");
            add_seperator_c(NULL, NULL, NULL, '-', NORMAL_SEPERATOR_SIZE);

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

            if (isalpha((unsigned char)buffer[0]) || delete_Base(base_list, id) == -1)
            {
                clear_terminal();
                print_colored(COLOR_ERROR, "Invalid input!\n", 1);
                goto Entry_no_clr;
            }
            else
            {
                print_colored(COLOR_SUCCESS, "List updated!\n", 1);
                goto Entry_delete_no_clr;
            }
            break;

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
    while ((cmd = GetCommand(CMD_C_LOAD | CMD_C_LOOP, NULL)) != CMD_BACK)
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
            add_seperator_c(NULL, ANSI_YELLOW, "delete ammo", '-', NORMAL_SEPERATOR_SIZE);
            printf("Use the following structure for the input:\n");
            printf("\t[I]   : id of ammo\n");
            add_seperator_c(NULL, NULL, NULL, '-', NORMAL_SEPERATOR_SIZE);
        Entry_delete_no_clr:

            char buffer[1024];
            int req = back_request(buffer, sizeof(buffer));
            if (req == 1)
                goto Entry;
            if (req == -1)
                goto Entry_delete_no_clr;

            buffer[strcspn(buffer, "\r\n")] = 0;
            unsigned id = (unsigned)strtoul(buffer, NULL, 10);

            if (delete_Ammo_t(ammo_t_list, id) == -1)
            {
                clear_terminal();
                print_colored(COLOR_ERROR, "Invalid ID!\n", 1);
                goto Entry_no_clr;
            }

            print_colored(COLOR_SUCCESS, "List updated!\n", 1);
            goto Entry_delete_no_clr;
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
        add_seperator_c(NULL, ANSI_YELLOW, "calculate", '-', NORMAL_SEPERATOR_SIZE);
        printf("%-39s <[%s]>\n", "What would you like to do?", "back");
        printf("\t- %-30s [%s]\n", "select parameters", "select");
        printf("\t- %-30s [%s]\n", "work with bases", "bases");
        printf("\t- %-30s [%s]\n", "work with target", "target");
        printf("\t- %-30s [%s]\n", "work with ammo", "ammo");
        add_seperator_c(NULL, NULL, NULL, '-', NORMAL_SEPERATOR_SIZE);
    }
    // waits for a valid command
    // on list, it exit's. In other cases it just returns, making it easier to make multiple calculations
    while ((cmd = GetCommand(CMD_C_LOOP | CMD_C_CALC | CMD_C_SELECT, NULL)) != CMD_BACK)
    {
        cmd_clr_ter(NULL, CMD_C_ALL, NULL);
        switch (cmd)
        {
        case CMD_SELECT:
            printf("Please input the base, target, ammo ID!\n");
            printf("If you aren't sure about the ID's, type [back], and check the [list] for the desired value!\n");
            printf("If you can't find, what you are looking for, then [add] it to the list!\n");
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
        printf("Are you sure, you want to save? ");
        cmd_help_inline(NULL, CMD_C_YESNO, NULL);
    }
    if (GetCommand(CMD_C_LOOP | CMD_C_YESNO, NULL) == CMD_NO)
        return;
    if (handle_save() == P_S_FALSE)
        printf("Atleast on save has failed!\n");
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
    printf(ANSI_GRAY " %-10s : " COLOR_TITLE "7.2.3\n", "Version");
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
    printf("\n");
    add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    printf(ANSI_GRAY " %-10s : " COLOR_TITLE "%d\n", "Terminal width", terminal_width());
    add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    printf(ANSI_RESET);
    printf("\n");
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
    while (command == CMD_INVALID)
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

        if (!(case_flags & CMD_C_LOOP))
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

// similiar to find_command_nontoken, but can work in tokened, and non token mode
int parse_command(char *input, Command_case flags, const char **cmd, size_t *cmd_len, const char **args)
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

// handle the command thing, calling the ptr, returning the cmd etc.
Command_t resolve_command(const char *cmd, size_t cmd_len, const char *args, Command_case flags, PROCESS *running)
{
    if (cmd)
    {
        // searches the commands list
        for (int i = 0; i < command_count; i++)
        {
            // checks the mask
            if (!(commands[i].case_mask & flags))
                continue; // if command not available in this mode, continue

            if (commands[i].name_len == cmd_len &&
                memcmp(cmd, commands[i].name, cmd_len) == 0)
            {
                // if the mode is set to no handlers, then it skips this part, used for NULL functions mostly
                if (commands[i].handler && !(flags & CMD_C_NOHANDLE))
                    commands[i].handler(args, flags, running);

                if ((CMD_C_LOOP & flags) &&
                    commands[i].type == CMD_HELP)
                    return CMD_INVALID;

                return commands[i].type;
            }
        }
        // error handling
        if (!(flags & CMD_C_NOCOMMENT))
            printf("Unknown command: [" ANSI_GREEN "%.*s" ANSI_RESET "]\n", (int)cmd_len, cmd);

        for (int i = 0; i < command_count; i++)
        {
            if (commands[i].type != CMD_HELP) // seraches for the help flag,
                continue;                     // to print out it's current name (only for keeping it dynamic)
            if (!(flags & CMD_C_NOCOMMENT) &&
                (flags & commands[i].case_mask))
                printf("If you need help, use the command [" ANSI_GREEN "%s" ANSI_RESET "]\n", commands[i].name);

            return CMD_INVALID;
        }
    }
    return CMD_INVALID;
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