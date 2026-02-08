#include "command\commands.h"
#include "command\list_wizard\wizard.h"
#include "helpers\color.h"
#include "helpers\helpers.h"
#include "memory\load.h"
#include "os\about_program.h"
#include "os\terminal_info.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Command table defining all available commands in the system
Command commands[] = {
    CMD_INIT("cancel", CMD_CANCEL, NULL, "Cancel event", CMD_C_CANCEL),
    CMD_INIT("select", CMD_SELECT, NULL, "Select choice", CMD_C_SELECT),
    CMD_INIT("list", CMD_LIST, NULL, "List values", CMD_C_LOAD),
    CMD_INIT("add", CMD_ADD, NULL, "Add a value", CMD_C_LOAD),
    CMD_INIT("delete", CMD_DELETE, NULL, "Delete a value", CMD_C_LOAD),
    CMD_INIT("back", CMD_BACK, NULL, "Step back", CMD_C_LOAD | CMD_C_SELECT | CMD_C_BACK),
    CMD_INIT("next", CMD_NEXT, NULL, "Switch to next", CMD_C_SWITCH),
    CMD_INIT("previous", CMD_PREV, NULL, "Switch to previous", CMD_C_SWITCH),

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

    CMD_INIT("yes", CMD_YES, NULL, "Yes", CMD_C_YESNO),
    CMD_INIT("no", CMD_NO, NULL, "No", CMD_C_YESNO)};

static const CommandAlias aliases[] = {
    {"y", CMD_YES},
    {"n", CMD_NO},
    {"b", CMD_BACK},
    {"nxt", CMD_NEXT},
    {"prv", CMD_PREV},
    {"prev", CMD_PREV},
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

//==================================================

size_t command_count = sizeof(commands) / sizeof(commands[0]);
size_t alias_count = sizeof(aliases) / sizeof(aliases[0]);
// sum of command_count & alias_count after init

CommandKey *command_keys = NULL;

static int register_key(size_t *key_pos, const char *key, Command *cmd)
{
    // Check for duplicates first
    for (size_t i = 0; i < *key_pos; i++)
    {
        if (command_keys[i].key && strcmp(command_keys[i].key, key) == 0)
        {
            fprintf(stderr, "Duplicate command key detected: '%s' used by commands '%s' and '%s'\n",
                    key, command_keys[i].cmd->name, cmd->name);
            return -1;
        }
    }

    // Add key to next available slot
    command_keys[(*key_pos)++] = (CommandKey){
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

    size_t command_key_count = 0; // will increment inside register_key

    // Register primary commands
    for (size_t i = 0; i < command_count; i++)
    {
        if (register_key(&command_key_count, commands[i].name, &commands[i]) < 0)
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

        if (register_key(&command_key_count, aliases[i].key, cmd) < 0)
            return S_TERMINATE;
    }

    return S_RUNNING;
}

void free_command_keys(void)
{
    free(command_keys);
    command_keys = NULL;
}

//==================================================

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

void cmd_help(const char *args, Command_case case_flags, PROCESS *running)
{
    // needed for the compiler
    (void)args;
    (void)running;

    // formated list
    if (!(case_flags & CMD_C_NOCOMMENT))
    {
        add_header(NORMAL_SEPERATOR_SIZE,
                   COLOR_HINT,
                   COLOR_TITLE "%-*s" COLOR_HINT " |" COLOR_TITLE "%-10s" COLOR_HINT " |" COLOR_TITLE "%s",
                   HELP_DESCRIPTION_ALIGMENT, "description", "command", "aliases");
    }
    else
        add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    for (size_t i = 0; i < command_count; i++) // goes trough the commands list, and print's out the aviable one
    {
        if (!(commands[i].case_mask & case_flags) || (commands[i].type == CMD_HELP))
            continue; // command not available in this mode

        printf(ANSI_GRAY " %-*s │" COLOR_TITLE "%-10s" ANSI_GRAY " │", HELP_DESCRIPTION_ALIGMENT, commands[i].description, commands[i].name);

        int alias_cnt = 0;

        for (size_t j = command_count; j < command_count + alias_count; j++)
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

void cmd_target(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)running;

    ListManagerConfig target_config = {
        .name = "target",
        .list_ptr = target_list,
        .print_ = (P_HRESULT (*)(ListManagerConfig *, DisplayType, void *, void *))target_info_print,
        .print_list = (void (*)(void *))print_Target_List,
        .find_by_id = (void *(*)(void *, unsigned))FindTargetByID,
        .delete_by_id = (int (*)(void *, unsigned))delete_Target};

    general_list_menu(case_flags, &target_config);
}

void print_Base_List_wrapper(Base_List *list)
{
    print_Base_List(list, S_RUNNING);
}

void cmd_base(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)running;

    ListManagerConfig base_config = {
        .name = "base",
        .list_ptr = base_list,
        .print_ = (P_HRESULT (*)(ListManagerConfig *, DisplayType, void *, void *))base_info_print,
        .print_list = (void (*)(void *))print_Base_List_wrapper,
        .on_add = (void (*)(ListManagerConfig *))base_add_wizard,      // NEW
        .find_by_id = (void *(*)(void *, unsigned))FindBaseByID,
        .delete_by_id = (int (*)(void *, unsigned))delete_BaseById};

    general_list_menu(case_flags, &base_config);
}

void cmd_ammo(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)running;


    ListManagerConfig ammo_config = {
        .name = "ammo_t",
        .list_ptr = ammo_t_list,
        .print_ = (P_HRESULT (*)(ListManagerConfig *, DisplayType, void *, void *))ammo_info_print,
        .print_list = (void (*)(void *))print_Ammo_t_List,
        .find_by_id = (void *(*)(void *, unsigned))FindAmmoByID,
        .delete_by_id = (int (*)(void *, unsigned))delete_Ammo_t};

    general_list_menu(case_flags, &ammo_config);
}

void cmd_test(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)case_flags;
    (void)running;
    run_pre_test(S_DEBUG);
}

void cmd_clr_ter(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)case_flags;
    (void)running;
    clear_terminal();
}

void cmd_config(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)case_flags;
    (void)running;
    recreate_config("config.txt", S_DEBUG);
}

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

void cmd_info(const char *args, Command_case case_flags, PROCESS *running)
{
    (void)args;
    (void)running;
    if (case_flags & CMD_C_NOCOMMENT)
        return;
    printf("\n");
    add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    printf(ANSI_GRAY " %-10s : " COLOR_TITLE P_NAME "\n", "Product");
    printf(ANSI_GRAY " %-10s : " COLOR_TITLE P_AUTHOR "\n", "Author");
    printf(ANSI_GRAY " %-10s : " COLOR_TITLE P_VERSION "\n", "Version");
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
    printf("Commands:\n");
    printf(ANSI_GRAY " %-15s : " COLOR_TITLE "%lld\n", "Count", command_count);
    printf(ANSI_GRAY " %-15s : " COLOR_TITLE "%lld\n", "Alias count", alias_count);
    putchar('\n');

    printf("Terminal:\n");
    int x, y;
    terminal_size(&x, &y);
    printf(ANSI_GRAY " %-15s : " COLOR_TITLE "%d\n", "Width", x);
    printf(ANSI_GRAY " %-15s : " COLOR_TITLE "%d\n", "Height", y);
    add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    printf(ANSI_RESET);
}

//==================================================
