#include "commands.h"
#include "error.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "calculations.h"

Command commands[] = {
    {"bases", CMD_BASE, cmd_base, "List bases", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_LOAD},
    {"targets", CMD_TARGET, cmd_target, "List targets", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_LOAD},
    {"test", CMD_TEST, cmd_test, "Do a manual self-test", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM},
    {"exit", CMD_EXIT, cmd_exit, "Exit program", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SYSTEM},
    {"help", CMD_HELP, cmd_help, "Show commands", CMD_C_ALL},
    {"colour", CMD_COLOUR, cmd_colour, "coloured mode command", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SETUP},
    {"mono", CMD_MONO, cmd_mono, "monochrome mode command", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_SETUP},
    {"yes", CMD_YES, NULL, "yes command", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_YESNO},
    {"no", CMD_NO, NULL, "no command", CMD_C_GENERIC | CMD_C_DEFAULT | CMD_C_YESNO}};

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
    for (int i = 0; i < command_count; i++)
    {
        if (!(commands[i].case_mask & case_flags) || (commands[i].type == CMD_HELP))
            continue; // command not available in this mode
        printf("    %-2s - %s\n", commands[i].name, commands[i].description);
    }
}

void cmd_target(const char *args, Command_case case_flags, PROCESS *running)
{
    if (running)
        *running = ListTargets();
}

void cmd_base(const char *args, Command_case case_flags, PROCESS *running)
{
    if (running)
        *running = ListBases();
}

void cmd_test(const char *args, Command_case case_flags, PROCESS *running)
{
    run_pre_test();
}

void cmd_colour(const char *args, Command_case case_flags, PROCESS *running)
{
    printf("Colour mode set to asdasdsadd\n");
}

void cmd_mono(const char *args, Command_case case_flags, PROCESS *running)
{
    printf("Colour mode set to idgaf\n");
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

void NormalizeInput(char *input)
{
    for (int i = 0; i < sizeof(input); i++)
        if (input[i] >= 'A' && input[i] <= 'Z')
            input[i] += 'a' - 'A';
}