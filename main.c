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

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <string.h>

#include "main.h"
#include "helpers\debug.h"
#include "command\commands.h"
#include "simulation\calculations.h"
#include "memory\load.h"
#include "error.h"
#include "memory\storage.h"
#include "helpers\color.h"
#include "helpers\helpers.h"
#include "os\input_poll.h"
#include "os\terminal_info.h"

// #include "debugmalloc.h"

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
#define DEFAULT_WIN_WIDTH 125 // 80 for calc
#define DEFAULT_WIN_HEIGHT 35

PROCESS Setup()
{
    printf("\nStarting setup...\n");

    if (build_command_keys() != S_RUNNING)
        return S_TERMINATE;

    enable_ansi_escape();

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    add_seperator_s(NULL, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    printf("\n");

    change_colour(ANSI_GRAY);
    // Step 1: Load or create configuration
    if (P_FAILED(test_and_run_config("config.txt", S_RUNNING)))
        return S_TERMINATE;
    printf("\n");

    // Step 2: Run system self-test
    if (P_FAILED(run_pre_test(S_RUNNING)))
        return S_TERMINATE;
    printf("\n");

    print_colored(COLOR_SUCCESS, "Setup finished!\n", 1);
    add_seperator_s(NULL, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);

    // Step 6: Adjust terminal size
    int x, y;
    terminal_size(&x, &y);
    if (x < DEFAULT_WIN_WIDTH || y < DEFAULT_WIN_HEIGHT)
    {
        printf(COLOR_WARNING "Terminal size [%d, %d] is smaller than advised, changing to [%d, %d]...\n" COLOR_RESET, x, y, DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT);
        printf("\033[8;%d;%dt", DEFAULT_WIN_HEIGHT, DEFAULT_WIN_WIDTH);
    }

    // Step 7: Clear terminal if color mode is enabled
    if (get_color_mode())
        clear_terminal();

    // Step 8: Display introduction banner
    add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    printf(ANSI_GRAY "Ballistic trajectory calculator\n");
    add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);

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
    // Free all dynamically allocated linked lists
    if (base_list)
        free_Base_List(&base_list);
    if (target_list)
        free_Target_List(&target_list);
    if (ammo_t_list)
        free_Ammo_t_List(&ammo_t_list);

    // Free global folder path
    free_folder_path();
    free_command_keys();
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
        Command_case main_flags = CMD_C_MODAL | CMD_C_GENERIC | CMD_C_SYSTEM | CMD_C_DEBUG;
        // Display available commands for current context
        cmd_help(NULL, main_flags, NULL);
        // Get and process user command, updating state if needed
        GetCommand(main_flags, &state);
    }

    // Perform cleanup before exit
    if (state != S_TERMINATE)
    {
        printf("Do you want to save before leaving? ");
        cmd_help_inline(NULL, CMD_C_YESNO, NULL);
        printf("\n");

        // Prompt for save (CMD_C_NOCOMMENT suppresses extra prompts in cmd_save)
        cmd_save(NULL, CMD_C_NOCOMMENT, NULL);
    }

    CleanUp();

    // Determine exit status based on final state
    if (state == S_END)
    {
        printf(COLOR_SUCCESS "Program exited succesfully!\n");
        return 0;
    }
    else if (state == S_TERMINATE)
    {
        fprintf(stderr, "Terminated with fatal error!\n");
        // Optional: Uncomment to keep terminal open for error inspection
        printf(COLOR_WARNING "Press any key, to close the window!\n");
        while (!input_available())
            Sleep(20);
        return -1;
    }

    return 0;
}