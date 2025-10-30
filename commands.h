#ifndef COMMANDS_H
#define COMMANDS_H

#include "error.h"

typedef enum
{
    CMD_EXIT,
    CMD_HELP,
    CMD_CONTINUE,
    CMD_YES,
    CMD_NO,
    CMD_CANCEL,
    CMD_RESET,
    CMD_TARGET,
    CMD_BASE,
    CMD_TEST,
} CommandType;

typedef struct
{
    const char *name;
    CommandType type;
    void (*handler)(const char *args, PROCESS *running);
    const char *description;
} Command;

//=====================================================

void GetCommand(PROCESS *running);
void NormalizeInput(char *input);

//=====================================================

void cmd_exit(const char *args, PROCESS *running);
void cmd_help(const char *args, PROCESS *running);
void cmd_base(const char *args, PROCESS *running);
void cmd_target(const char *args, PROCESS *running);
void cmd_test(const char *args, PROCESS *running);

//=====================================================

extern Command commands[];

extern int command_count;

#endif