#include "commands.h"
#include "error.h"
#include "debug.h"
#include "calculations.h"
#include "color.h"
#include "load.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debugmalloc.h"

Command commands[] = {
    {"cancel", CMD_CANCEL, NULL, "Cancel event", CMD_C_LOAD},
    {"list", CMD_LIST, NULL, "list values", CMD_C_LOAD},
    {"add", CMD_ADD, NULL, "add value", CMD_C_LOAD},
    {"delete", CMD_DELETE, NULL, "delete value", CMD_C_LOAD},

    {"bases", CMD_BASE, cmd_base, "List bases", CMD_C_GENERIC | CMD_C_DEFAULT},
    {"targets", CMD_TARGET, cmd_target, "List targets", CMD_C_GENERIC | CMD_C_DEFAULT},
    {"ammo", CMD_AMMO, cmd_ammo, "List ammo", CMD_C_GENERIC | CMD_C_DEFAULT},

    {"test", CMD_TEST, cmd_test, "Do a manual self-test", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM},
    {"clear", CMD_CLEAR_TERMINAL, cmd_clr_ter, "Clears the terminal", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM},
    {"config", CMD_TEST, cmd_config, "Modify config", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM},
    {"exit", CMD_EXIT, cmd_exit, "Exit program", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM},
    {"help", CMD_HELP, cmd_help, "Show commands", CMD_C_ALL},

    {"colour", CMD_COLOUR, cmd_colour, "coloured mode command", CMD_C_SETUP},
    {"mono", CMD_MONO, cmd_mono, "monochrome mode command", CMD_C_SETUP},

    {"yes", CMD_YES, NULL, "yes command", CMD_C_YESNO},
    {"no", CMD_NO, NULL, "no command", CMD_C_YESNO}};

/*calculates the number of commands, the division with the first value is needed,
because we devide the whole size, with one entry's size, and by that we get the number of entry's*/
int command_count = sizeof(commands) / sizeof(commands[0]);

//================================================

void cmd_exit(const char *args, Command_case case_flags, PROCESS *running)
{
    printf("Exiting...\n");
    if (running)
        *running = S_END;
    else
    {
        printf("Running pointer error\n");
    }
}

void cmd_help(const char *args, Command_case case_flags, PROCESS *running)
{
    // clear_terminal();
    add_seperator("command list", '-', 30);

    for (int i = 0; i < command_count; i++)
    {
        if (!(commands[i].case_mask & case_flags) || (commands[i].type == CMD_HELP))
            continue; // command not available in this mode
        printf("    %-4s\t- %s\n", commands[i].name, commands[i].description);
    }
    add_seperator(NULL, '-', 30);
}

void cmd_target(const char *args, Command_case case_flags, PROCESS *running)
{
    printf("%-39s <[%s]>\n", "What would you like to do?", "cancel");
    printf("\t- %-30s [%s]\n", "show target list", "list");
    printf("\t- %-30s [%s]\n", "add target to list", "add");
    printf("\t- %-30s [%s]\n", "remove target from list", "delete");

    Command_t cmd = GetCommand(CMD_C_LOAD | CMD_C_LOOP, NULL);
    switch (cmd)
    {
    case CMD_LIST:
        print_Target_List(target_list);
        break;
    case CMD_ADD:
        EMPTY_MESSAGE
        break;
    case CMD_DELETE:
        EMPTY_MESSAGE
        break;

    default:
        break;
    }
}

void cmd_base(const char *args, Command_case case_flags, PROCESS *running)
{
    printf("%-39s <[%s]>\n", "What would you like to do?", "cancel");
    printf("\t- %-30s [%s]\n", "show base list", "list");
    printf("\t- %-30s [%s]\n", "add base to list", "add");
    printf("\t- %-30s [%s]\n", "remove base from list", "delete");
    Command_t cmd = GetCommand(CMD_C_LOAD | CMD_C_LOOP, NULL);
        
    switch (cmd)
    {
    case CMD_LIST:
        print_Base_List(base_list, S_DEBUG);
        break;
    case CMD_ADD:
        EMPTY_MESSAGE
        break;
    case CMD_DELETE:
        EMPTY_MESSAGE
        break;

    default:
        break;
    }
}

void cmd_ammo(const char *args, Command_case case_flags, PROCESS *running)
{
    printf("%-39s <[%s]>\n", "What would you like to do?", "cancel");
    printf("\t- %-30s [%s]\n", "show ammo type list", "list");
    printf("\t- %-30s [%s]\n", "add ammo type to list", "add");
    printf("\t- %-30s [%s]\n", "remove ammo type from list", "delete");

    Command_t cmd = GetCommand(CMD_C_LOAD | CMD_C_LOOP, NULL);
    switch (cmd)
    {
    case CMD_LIST:
        print_Ammo_t_List(ammo_t_list);
        break;
    case CMD_ADD:
        EMPTY_MESSAGE
        break;
    case CMD_DELETE:
        EMPTY_MESSAGE
        break;

    default:
        break;
    }
}

void cmd_test(const char *args, Command_case case_flags, PROCESS *running)
{

    run_pre_test(S_DEBUG);
}

void cmd_colour(const char *args, Command_case case_flags, PROCESS *running)
{
    set_color_mode(CMD_COLOUR);
}

void cmd_mono(const char *args, Command_case case_flags, PROCESS *running)
{
    set_color_mode(CM_MONO);
}

// clear terminal command
void cmd_clr_ter(const char *args, Command_case case_flags, PROCESS *running)
{
    if (get_color_mode())
        clear_terminal();
}

void cmd_config(const char *args, Command_case case_flags, PROCESS *running)
{
    recreate_config("config.txt", S_DEBUG);
}
//================================================

Command_t GetCommand(Command_case case_flags, PROCESS *running)
{
    char input[128];
    int modal = 1;
    Command_t command = CMD_INVALID;

    while (modal)
    {
        printf("> ");
        if (fgets(input, (sizeof(input) > 128) ? 128 : sizeof(input), stdin))
        {
            NormalizeInput(input);
            char *cmd = strtok(input, " \t\n");
            char *args = strtok(NULL, "\n");

            if (cmd)
            {
                int found = 0;
                for (int i = 0; i < command_count; i++)
                {
                    if (!(commands[i].case_mask & case_flags))
                        continue; // command not available in this mode

                    if (strcmp(cmd, commands[i].name) == 0)
                    {
                        found = 1;

                        if (commands[i].type != CMD_HELP) // so help doesn't trigger the finsih command
                            modal = 0;

                        if (case_flags & CMD_C_GENERIC)
                            command = commands[i].type;

                        if (commands[i].handler)
                            commands[i].handler(args, case_flags, running);

                        command = commands[i].type;
                    }
                }
                if (!found)
                {
                    for (int i = 0; i < command_count; i++)
                    {
                        if (commands[i].type != CMD_HELP) // seraches for the help flag,
                            continue;                     // to print out it's current name (only for keeping it dynamic)
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

// later make it enum dependent
void add_seperator(const char *str, char symbol, unsigned sep_length)
{
    size_t str_length = (str) ? strlen(str) : 0;
    int start = (sep_length - str_length) / 2;
    int cnt = 0;

    for (cnt = 0; cnt < start; cnt++)
    {
        printf("%c", symbol);
    };
    if (str)
        printf("%s", str);
    for (cnt += str_length; cnt < sep_length; cnt++)
    {
        printf("%c", symbol);
    };
    printf("\n");
}

void clear_terminal()
{
    if (get_color_mode() == CM_COLOUR)
        printf("\033[2J\033[H");
};
