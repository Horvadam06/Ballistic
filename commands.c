#include "commands.h"
#include "error.h"
#include "debug.h"
#include "calculations.h"
#include "color.h"
#include "load.h"
#include "helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debugmalloc.h"

//========================================================================
// GLOBAL COMMAND TABLE
//========================================================================

// Command table defining all available commands in the system
// Format: {command_name, command_type, handler_function, description, context_flags}
// NOTE: Commands with NULL handlers are handled specially by GetCommand()
Command commands[] = {
    // Navigation and basic operations - these have NULL handlers as they're
    // handled directly in modal contexts (like cmd_base, cmd_target loops)
    {"back", CMD_BACK, NULL, "step back", CMD_C_LOAD | CMD_C_SELECT | CMD_C_BACK},
    {"cancel", CMD_CANCEL, NULL, "Cancel event", CMD_C_CANCEL},
    {"select", CMD_SELECT, NULL, "select choice", CMD_C_SELECT},
    {"list", CMD_LIST, NULL, "list values", CMD_C_LOAD},
    {"add", CMD_ADD, NULL, "add value", CMD_C_LOAD},
    {"delete", CMD_DELETE, NULL, "delete value", CMD_C_LOAD},

    {"bases", CMD_BASE, cmd_base, "List bases", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_CALC},
    {"targets", CMD_TARGET, cmd_target, "List targets", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_CALC},
    {"ammo", CMD_AMMO, cmd_ammo, "List ammo", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_CALC},
    {"calc", CMD_CALCULATE, cmd_calculate, "calculate trajectory", CMD_C_GENERIC | CMD_C_DEFAULT},

    // clear's the list, so it gives you a fresh start
    {"test", CMD_TEST, cmd_test, "Do a manual self-test, ONLY do when debugging", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM},
    {"clear", CMD_CLEAR_TERMINAL, cmd_clr_ter, "Clears the terminal", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM},
    {"config", CMD_TEST, cmd_config, "Modify config", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM},
    {"save", CMD_SAVE, cmd_save, "save changes", CMD_C_EXIT | CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM},
    {"exit", CMD_EXIT, cmd_exit, "Exit program", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM},
    {"help", CMD_HELP, cmd_help, "Show commands", CMD_C_ALL},

    {"colour", CMD_COLOUR, cmd_colour, "coloured mode command", CMD_C_SETUP},
    {"mono", CMD_MONO, cmd_mono, "monochrome mode command", CMD_C_SETUP},

    {"yes", CMD_YES, NULL, "yes command", CMD_C_YESNO},
    {"no", CMD_NO, NULL, "no command", CMD_C_YESNO}};

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

    printf("Are you sure you want to exit? [yes/no]\n");
    if (GetCommand(CMD_C_LOOP | CMD_C_YESNO, NULL) == CMD_NO)
        return;

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
 * Note: add_seperator() is assumed to handle NULL title for closing separator
 */
void cmd_help(const char *args, Command_case case_flags, PROCESS *running)
{
    // needed for the compiler
    (void)args;
    (void)running;

    // formated list
    add_seperator("command list", '-', 30);
    for (int i = 0; i < command_count; i++) // goes trough the commands list, and print's out the aviable one
    {
        if (!(commands[i].case_mask & case_flags) || (commands[i].type == CMD_HELP))
            continue; // command not available in this mode
        printf("    %-4s\t- %s\n", commands[i].name, commands[i].description);
    }
    add_seperator(NULL, '-', 30);
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
    // needed for the compiler
    (void)args;
    (void)running;

    // formated menu options
    cmd_clr_ter(NULL, CMD_C_ALL, NULL);
    if (!(case_flags & CMD_C_NOCOMMENT))
    {
        add_seperator("targets", '-', 30);
        printf("%-39s <[%s]>\n", "What would you like to do?", "back");
        printf("\t- %-30s [%s]\n", "show target list", "list");
        printf("\t- %-30s [%s]\n", "add target to list", "add");
        printf("\t- %-30s [%s]\n", "remove target from list", "delete");
        add_seperator(NULL, '-', 30);
    }

    Command_t cmd;
    // waits for a valid command
    // on list, it exit's. In other cases it just returns, making it easier to input multiple parameters
    while ((cmd = GetCommand(CMD_C_LOAD | CMD_C_LOOP, NULL)) != CMD_BACK)
        switch (cmd)
        {

        case CMD_LIST:
            print_Target_List(target_list);
            return;
        case CMD_ADD:
            char buffer[1024];
            fgets(buffer, sizeof(buffer), stdin);
            handle_target(buffer, target_list, NULL);
            break;
        case CMD_DELETE:
            unsigned id;
            scanf("%u", &id);
            delete_Target(target_list, id);
            break;

        default:
            break;
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
    cmd_clr_ter(NULL, CMD_C_ALL, NULL);
    if (!(case_flags & CMD_C_NOCOMMENT))
    {
        add_seperator("bases", '-', 30);
        printf("%-39s <[%s]>\n", "What would you like to do?", "back");
        printf("\t- %-30s [%s]\n", "show base list", "list");
        printf("\t- %-30s [%s]\n", "add base to list", "add");
        printf("\t- %-30s [%s]\n", "remove base from list", "delete");
        add_seperator(NULL, '-', 30);
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
            char buffer[1024];
            fgets(buffer, sizeof(buffer), stdin);
            handle_base(buffer, base_list, NULL);
            break;
        case CMD_DELETE:
            unsigned id;
            scanf("%u", &id);
            delete_Base(base_list, id);
            break;

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
    cmd_clr_ter(NULL, CMD_C_ALL, NULL);
    if (!(case_flags & CMD_C_NOCOMMENT))
    {
        add_seperator("ammo", '-', 30);
        printf("%-39s <[%s]>\n", "What would you like to do?", "back");
        printf("\t- %-30s [%s]\n", "show ammo type list", "list");
        printf("\t- %-30s [%s]\n", "add ammo type to list", "add");
        printf("\t- %-30s [%s]\n", "remove ammo type from list", "delete");
        add_seperator(NULL, '-', 30);
    }

    Command_t cmd;
    // waits for a valid command
    // on list, it exit's. In other cases it just returns, making it easier to input multiple parameters
    while ((cmd = GetCommand(CMD_C_LOAD | CMD_C_LOOP, NULL)) != CMD_BACK)
        switch (cmd)
        {
        case CMD_LIST:
            print_Ammo_t_List(ammo_t_list);
            return;
        case CMD_ADD:
            char buffer[1024];
            fgets(buffer, sizeof(buffer), stdin);
            handle_ammo_t(buffer, ammo_t_list, NULL);
            break;
        case CMD_DELETE:
            unsigned id;
            scanf("%u", &id);
            delete_Ammo_t(ammo_t_list, id);
            break;

        default:
            break;
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
        add_seperator("calculate", '-', 30);
        printf("%-39s <[%s]>\n", "What would you like to do?", "back");
        printf("\t- %-30s [%s]\n", "select parameters", "select");
        printf("\t- %-30s [%s]\n", "work with bases", "bases");
        printf("\t- %-30s [%s]\n", "work with target", "target");
        printf("\t- %-30s [%s]\n", "work with ammo", "ammo");
        add_seperator(NULL, '-', 30);
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
        printf("Are you sure, you want to save? [yes/no]");
    if (GetCommand(CMD_C_LOOP | CMD_C_YESNO, NULL) == CMD_NO)
        return;
    if (handle_save() == P_S_FALSE)
        printf("Atleast on save has failed!\n");
}

//========================================================================
// COMMAND PROCESSING CORE
//========================================================================

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
    int modal = 1;
    Command_t command = CMD_INVALID;

    // if case_flags CMD_C_LOOP has been set, than the function run's till a valid input
    while (modal)
    {
        printf("> ");
        if (fgets(input, (sizeof(input) > 128) ? 128 : sizeof(input), stdin))
        {
            // set's all the charcter to low letter ones, if not already
            NormalizeInput(input);
            // the tokenized command parameter
            char *cmd = strtok(input, " \t\n");
            // data/arguments to bass to the handlers
            char *args = strtok(NULL, "\n");

            if (cmd)
            {
                int found = 0;
                // searches the commands list
                for (int i = 0; i < command_count; i++)
                {
                    // checks the mask
                    if (!(commands[i].case_mask & case_flags))
                        continue; // if command not available in this mode, continue

                    if (strcmp(cmd, commands[i].name) == 0)
                    {
                        found = 1;

                        if (commands[i].type != CMD_HELP) // so help doesn't trigger the finsih command
                            modal = 0;

                        if (case_flags & CMD_C_GENERIC)
                            command = commands[i].type;

                        // if the mode is set to no handlers, then it skips this part, used for NULL functions mostly
                        if (commands[i].handler && !(case_flags & CMD_C_NOHANDLE))
                            commands[i].handler(args, case_flags, running);

                        command = commands[i].type;
                    }
                }
                // error handling
                if (!found)
                {
                    for (int i = 0; i < command_count; i++)
                    {
                        if (commands[i].type != CMD_HELP) // seraches for the help flag,
                            continue;                     // to print out it's current name (only for keeping it dynamic)
                        if (!(case_flags & CMD_C_NOCOMMENT))
                            printf("Unknown command: [%s]\n If you need help, use the command [%s]\n", cmd, commands[i].name);

                        if (!(case_flags & CMD_C_LOOP))
                            return CMD_INVALID;
                    }
                }
            }
        }
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

    char *cmd = strtok(input, " \t\n");
    char *args = strtok(NULL, "\n");

    if (!cmd)
        return CMD_INVALID;

    int found = 0;

    for (int i = 0; i < command_count; i++)
    {
        if (!(commands[i].case_mask & case_flags))
            continue; // command not available in this mode

        if (strcmp(cmd, commands[i].name) == 0)
        {
            found = 1;

            if (commands[i].handler && !(case_flags & CMD_C_NOHANDLE))
                commands[i].handler(args, case_flags, running);

            return commands[i].type;
        }
    }

    if (!found)
    {
        // Only show help for commands valid in this mode
        for (int i = 0; i < command_count; i++)
        {
            if (!(commands[i].case_mask & case_flags))
                continue;

            if (commands[i].type == CMD_HELP)
            {
                if (!(case_flags & CMD_C_NOCOMMENT))
                    printf("Unknown command: [%s]\n If you need help, use the command [%s]\n", cmd, commands[i].name);
                break;
            }
        }
    }

    return CMD_INVALID;
}
