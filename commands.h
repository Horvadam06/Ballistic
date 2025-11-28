#ifndef COMMANDS_H
#define COMMANDS_H

#include "error.h"
#include <stdint.h>

typedef enum
{
    CMD_INVALID,
    CMD_EXIT,
    CMD_HELP,
    CMD_CONTINUE,
    CMD_YES,
    CMD_NO,
    CMD_CANCEL,
    CMD_RESET,
    CMD_TARGET,
    CMD_BASE,
    CMD_AMMO,
    CMD_TEST,
    CMD_COLOUR,
    CMD_MONO,
    CMD_CLEAR_TERMINAL
} Command_t;

/*=====command_case list==========
CMD_C_LOAD:
    -CMD_TARGET
    -CMD_BASE
CMD_C_SYSTEM:
    -CMD_EXIT
    -CMD_HELP
*/

typedef enum
{                           // all of them has the input as return values
                            // to make them stackable I use bit masking
    CMD_C_DEFAULT = 1 << 0, // acces for the full list of commands, uses the handlers
    CMD_C_GENERIC = 1 << 1, // acces for the full list of commands, DOESN'T uses the handlers
    CMD_C_SYSTEM = 1 << 2,  // only has acces to the system specific commands, uses the handlers
    CMD_C_YESNO = 1 << 3,   // only has acces for the yes no commands, doesn't uses the handlers
    CMD_C_LOAD = 1 << 4,    // only has acceses for the loading specific commands, uses the handlers
    CMD_C_SETUP = 1 << 5,   // only has acceses for the loading specific commands, uses the handlers
    CMD_C_LOOP = 1 << 20    // special control flag, make's the get command loop until it get's a valid input for it's case

} Command_case;

#define CMD_C_ALL 0xFFFFFFFFu

typedef struct
{
    const char *name;
    Command_t type;
    void (*handler)(const char *args, Command_case case_flags, PROCESS *running);
    const char *description;
    uint32_t case_mask;
} Command;

//=====================================================

Command_t GetCommand(Command_case case_flags, PROCESS *running);
void NormalizeInput(char *input);

void add_seperator(const char *str, char symbol, unsigned sep_length);
void clear_terminal();

//=====================================================

void cmd_exit(const char *args, Command_case case_flags, PROCESS *running);
void cmd_help(const char *args, Command_case case_flags, PROCESS *running);
void cmd_base(const char *args, Command_case case_flags, PROCESS *running);
void cmd_ammo(const char *args, Command_case case_flags, PROCESS *running);
void cmd_target(const char *args, Command_case case_flags, PROCESS *running);
void cmd_test(const char *args, Command_case case_flags, PROCESS *running);
void cmd_config(const char *args, Command_case case_flags, PROCESS *running);
void cmd_colour(const char *args, Command_case case_flags, PROCESS *running);
void cmd_mono(const char *args, Command_case case_flags, PROCESS *running);
void cmd_clr_ter(const char *args, Command_case case_flags, PROCESS *running);

//=====================================================

extern Command commands[];

extern int command_count;

#endif