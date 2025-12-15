/**
 * @file main.c
 * @brief Main application entry point and lifecycle management
 *
 * This file contains the core application logic that coordinates:
 * - Initialization and setup
 * - Main command processing loop
 * - Cleanup and resource management
 *
 * @note Project created: 2025.10.17
 * @note Project finished: 2025.12.02
 * @author Horvath Adam Jozsef
 *
 * @todo Consider adding command-line argument support
 * @todo Add error recovery for partial data loading failures
 */

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
#include "helpers.h"

#include "debugmalloc.h"

//=========================================
// Created by.: Horvath Adam Jozsef
// Starting date: 2025.10.17
// Finishing date: 2025.12.02
//=========================================

/**
 * @brief Initializes the application and loads required data
 *
 * Performs the complete startup sequence including:
 * 1. Configuration file loading/creation
 * 2. System self-test
 * 3. Data file loading (ammo, bases, targets)
 * 4. Display initialization and welcome message
 *
 * @return PROCESS S_RUNNING on success, S_TERMINATE on failure
 *
 * @note Called once at program startup before main loop
 * @note Sets up global data structures (ammo_t_list, base_list, target_list)
 * @note Handles both fresh install (creates config) and existing install scenarios
 */
PROCESS Setup()
{
    printf("Starting setup...\n");

    // Step 1: Load or create configuration
    if (P_FAILED(test_and_run_config("config.txt", S_RUNNING)))
        return S_TERMINATE;

    // Step 2: Run system self-test
    if (P_FAILED(run_pre_test(S_RUNNING)))
        return S_TERMINATE;

    printf("Setup finished!\n");

    char folder_buffer[1024];

    // Step 3: Load ammunition catalogue (if not already loaded by pre-test)
    if (!ammo_t_list)
    {
        strcpy(folder_buffer, folder_path);
        strcat(folder_buffer, "ammo_catalouge.txt");
        init_Ammo_t_List(&ammo_t_list);
        scan(folder_buffer, handle_ammo_t, ammo_t_list, S_RUNNING);
    }

    // Step 4: Load base list (if not already loaded by pre-test)
    if (!base_list)
    {
        strcpy(folder_buffer, folder_path);
        strcat(folder_buffer, "bases.txt");
        init_Base_List(&base_list);
        scan(folder_buffer, handle_base, base_list, S_RUNNING);
    }

    // Step 5: Load target list (if not already loaded by pre-test)
    if (!target_list)
    {
        strcpy(folder_buffer, folder_path);
        strcat(folder_buffer, "targets.txt");
        init_Target_List(&target_list);
        scan(folder_buffer, handle_target, target_list, S_RUNNING);
    }

    // Step 6: Clear terminal if color mode is enabled
    if (get_color_mode())
    {
        clear_terminal();
    }

    // Step 7: Display introduction banner
    printf("\n");
    printf("==================================\n");
    printf("Ballistic trajectory calculator\n");
    printf("Created by.: Adam Jozsef Horvath\n");
    printf("==================================\n");
    printf("\n");
    printf("Type [exit] to leave or type [help] for the commands!\n");

    return S_RUNNING;
}

/**
 * @brief Cleans up resources and saves data before exit
 *
 * Performs graceful shutdown sequence:
 * 1. Prompts user to save changes
 * 2. Frees all allocated memory (linked lists)
 * 3. Cleans up global resources
 *
 * @note Called once at program termination
 * @note User can choose to skip saving if desired
 * @warning Must be called to prevent memory leaks
 */
void CleanUp()
{
    printf("Do you want to save before leaving? [yes/no]\n");

    // Prompt for save (CMD_C_NOCOMMENT suppresses extra prompts in cmd_save)
    cmd_save(NULL, CMD_C_NOCOMMENT, NULL);

    // Free all dynamically allocated linked lists
    if (base_list)
        free_Base_List(&base_list);
    if (target_list)
        free_Target_List(&target_list);
    if (ammo_t_list)
        free_Ammo_t_List(&ammo_t_list);

    // Free global folder path
    free_folder_path();
}

/**
 * @brief Main entry point of the application
 *
 * Manages the complete program lifecycle:
 * 1. Setup (configuration, data loading)
 * 2. Main command loop (user interaction)
 * 3. Cleanup (saving, resource deallocation)
 *
 * @return int 0 on normal exit, -1 on error termination
 *
 * @note Uses state machine pattern (PROCESS enum)
 * @note Main loop continues until S_END or S_TERMINATE state
 * @note Debugmalloc is used for memory leak detection in debug builds
 */
int main(void)
{
    // Initialize application state and load data
    PROCESS state = Setup();

    // Main application loop - runs while in running state
    while (state == S_RUNNING)
    {
        // Display available commands for current context
        cmd_help(NULL, CMD_C_DEFAULT | CMD_C_GENERIC | CMD_C_SYSTEM, NULL);

        // Get and process user command, updating state if needed
        GetCommand(CMD_C_LOOP | CMD_C_GENERIC | CMD_C_SYSTEM, &state);
    }

    // Perform cleanup before exit
    CleanUp();

    // Determine exit status based on final state
    if (state == S_END)
    {
        printf("Program exited succesfully!\n");
        return 0;
    }
    else if (state == S_TERMINATE)
    {
        printf("Program terminated!\n");
        // Optional: Uncomment to keep terminal open for error inspection
        printf("Press any key, to close the window!\n");
        getchar(); //maybe use it, so the terminal is left open till some keypress
        return -1;
    }

    return 0;
}