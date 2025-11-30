#include <stdio.h>
#include <string.h>

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
    if (FAILED(test_and_run_config("config.txt", S_RUNNING)))
        return S_TERMINATE;

    if (FAILED(run_pre_test(S_RUNNING)))
        return S_TERMINATE;

    printf("Setup finished!\n");

    init_Ammo_t_List(&ammo_t_list);

    char folder_buffer[1024];
    strcpy(folder_buffer, folder_path);
    strcat(folder_buffer, "ammo_catalouge.txt");
    scan(folder_buffer, handle_ammo_t, ammo_t_list, S_DEBUG);
    print_Ammo_t_List(ammo_t_list);
    free_Ammo_t_List(&ammo_t_list);

    if (get_color_mode())
    {
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

    CleanUp();

    if (state == S_END)
    {
        printf("Program exited succesfully!\n");
        return 0;
    }
    else if (state == S_TERMINATE)
    {
        printf("Program terminated!\n");
        // getchar(); maybe use it, so the terminal is left open till some keypress
        return -1;
    }

    return 0;
}