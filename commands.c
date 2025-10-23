#include "commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "calculations.h"

Command commands[] = {
    {"bases", CMD_BASE, cmd_base, "List bases"},
    {"targets", CMD_TARGET, cmd_target, "List targets"},
    {"exit", CMD_EXIT, cmd_exit, "Exit program"},
    {"help", CMD_HELP, cmd_help, "Show commands"},
    // non list commands
    {"yes", CMD_YES, cmd_help, "yes command"},
    {"no", CMD_NO, cmd_help, "no command"}};

int command_count = sizeof(commands) / sizeof(commands[0]);

//================================================

void cmd_exit(const char *args, int *running)
{
    printf("Exiting...\n");
    if (running)
        *running = 0;
    else
    {
        printf("Running pointer error\n");
    }
}

void cmd_help(const char *args, int *running)
{
    for (int i = 0; i < command_count; i++)
    {
        printf("%-2s - %s\n", commands[i].name, commands[i].description);
        if (commands[i].type == CMD_HELP)
            break;
    }
}

void cmd_target(const char *args, int *running)
{
    *running = ListTargets();
}

void cmd_base(const char *args, int *running)
{
    *running = ListBases();
}

//================================================

void GetCommand(int *running)
{
    char input[128];

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
                if (strcmp(cmd, commands[i].name) == 0)
                {
                    commands[i].handler(args, running);
                    found = 1;
                    break;
                }
            }
            if (!found)
            {
                for (int i = 0; i < command_count; i++)
                {
                    if (commands[i].type != CMD_HELP) continue;
                    printf("Unknown command: [%s]\n If you need help, use the command [%s]\n", cmd, commands[i].name);
                    break;
                }
                
            }
        }
    }
}

void NormalizeInput(char *input)
{
    for (int i = 0; i < sizeof(input); i++)
        if (input[i] >= 'A' && input[i] <= 'Z')
            input[i] += 'a' - 'A';
}