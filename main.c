#include <stdio.h>

#include "main.h"
#include "debug.h"
#include "commands.h"
#include "calculations.h"
#include "load.h"
#include "error.h"
#include "storage.h"
#include "color.h"

//=========================================
// Created by.:
// Starting date: 2025.10.17
// Finishing date:
//=========================================

PROCESS Setup()
{
    printf("Starting setup...\n");
    if (!SUCCEEDED(test_and_run_config("config.txt", S_RUNNING)))
        return S_TERMINATE;

    if (!SUCCEEDED(run_pre_test(S_RUNNING)))
        return S_TERMINATE;

    printf("Setup finished!\n");

    if (get_color_mode()){
        clear_terminal();
    }

    printf("\n");
    printf("==================================\n");
    printf("Ballistic trajectory calculator\n");
    printf("Created by.: Adam Jozsef Horvath\n");
    printf("==================================\n");
    printf("\n");
    printf("Type [exit] to leave or type [help] for the commands!\n");

    return S_RUNNING;
}

void CleanUp()
{
    free_folder_path();
}

int main(void)
{
    PROCESS state = Setup();
    
    while (state == S_RUNNING)
    {
        GetCommand(CMD_C_LOAD | CMD_C_SYSTEM, &state);
    }
    if (state == S_END)
        printf("Program exited succesfully!\n");
    else if (state == S_TERMINATE)
        printf("Program terminated!\n");

    CleanUp();
    return 0;
}