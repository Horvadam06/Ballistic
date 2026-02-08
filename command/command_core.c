#include "command\commands.h"
#include "helpers\color.h"
#include "helpers\helpers.h"

#include <stdio.h>
#include <string.h>

//==================================================
//Private helpers
//==================================================

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

static Command *lookup_command(const char *cmd, size_t len, Command_case flags)
{
    for (size_t i = 0; i < command_count + alias_count; i++)
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

//==================================================

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