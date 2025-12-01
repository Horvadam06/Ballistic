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

#include "debugmalloc.h"

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

    char folder_buffer[1024];
    strcpy(folder_buffer, folder_path);
    strcat(folder_buffer, "ammo_catalouge.txt");
    init_Ammo_t_List(&ammo_t_list);
    scan(folder_buffer, handle_ammo_t, ammo_t_list, S_RUNNING);
    // print_Ammo_t_List(ammo_t_list);

    strcpy(folder_buffer, folder_path);
    strcat(folder_buffer, "bases.txt");
    init_Base_List(&base_list);
    scan(folder_buffer, handle_base, base_list, S_RUNNING);
    // print_Base_List(base_list, S_RUNNING);

    strcpy(folder_buffer, folder_path);
    strcat(folder_buffer, "targets.txt");
    init_Target_List(&target_list);
    scan(folder_buffer, handle_target, target_list, S_RUNNING);
    // print_Target_List(target_list);

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
    cmd_help(NULL, CMD_HELP, NULL);
    return S_RUNNING;
}

void CleanUp()
{
    printf("Do you want to save before leaving? [YES/NO]\n");
    if (GetCommand(CMD_C_YESNO | CMD_C_LOOP, NULL) == CMD_YES)
    {
        char folder_buffer[1024];
        if (base_list)
        {
            strcpy(folder_buffer, folder_path);
            strcat(folder_buffer, "bases.txt");
            if (SUCCEEDED(Save_BaseList(base_list, folder_buffer)))
                printf("Base list saved succesfully!\n");
        }

        if (target_list)
        {
            strcpy(folder_buffer, folder_path);
            strcat(folder_buffer, "targets.txt");
            if (SUCCEEDED(Save_TargetList(target_list, folder_buffer)))
                printf("Target list saved succesfully!\n");
        }

        if (ammo_t_list)
        {
            strcpy(folder_buffer, folder_path);
            strcat(folder_buffer, "ammo_catalouge.txt");
            if (SUCCEEDED(Save_Ammo_t_List(ammo_t_list, folder_buffer)))
                printf("Ammo Type list saved succesfully!\n");
        }
    }
    if (base_list)
        free_Base_List(&base_list);
    if (target_list)
        free_Target_List(&target_list);
    if (ammo_t_list)
        free_Ammo_t_List(&ammo_t_list);

    free_folder_path();
}

int main(void)
{
    PROCESS state = Setup();

    while (state == S_RUNNING)
    {
        GetCommand(CMD_C_GENERIC | CMD_C_SYSTEM, &state);
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