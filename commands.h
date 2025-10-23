#ifndef COMMANDS_H
#define COMMANDS_H

typedef enum {
    CMD_EXIT,
    CMD_HELP,
    CMD_CONTINUE,
    CMD_YES,
    CMD_NO,
    CMD_CANCEL,
    CMD_RESET,
    CMD_TARGET,
    CMD_BASE,
} CommandType;

typedef struct {
    const char *name;
    CommandType type;
    void (*handler)(const char *args, int *running);
    const char *description;
} Command;


//=====================================================

void GetCommand(int *running);
void NormalizeInput(char* input);

//=====================================================

void cmd_exit(const char *args, int *running);
void cmd_help(const char *args, int *running);
void cmd_base(const char *args, int *running);
void cmd_target(const char *args, int *running);

//=====================================================

extern Command commands[];

extern int command_count;

#endif