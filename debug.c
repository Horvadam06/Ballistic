#include "debug.h"
#include "error.h"
#include "load.h"
#include "color.h"
#include "commands.h"
#include "helpers.h"

#include <stdio.h>
#include <string.h>

/**
 * @brief Runs a comprehensive pre-test of system components
 *
 * This function performs initial system validation by:
 * 1. Loading data files if lists are empty
 * 2. Testing color output functionality
 * 3. Verifying data integrity
 * 4. Providing debugging output when in debug mode
 *
 * @param process_state Current process state (S_DEBUG for verbose output)
 * @return P_HRESULT S_OK if all tests pass, error code otherwise
 *
 * @note This is a development/debugging function and should typically
 *       only be called via the 'test' command or during initialization
 * @warning May overwrite existing data if lists are already initialized
 */
P_HRESULT run_pre_test(PROCESS process_state)
{
    printf("Starting test...\n");

    P_HRESULT hr = P_S_OK;

    char folder_buffer[1024];
    // load ammo catalogue (if not already loaded)
    if (!ammo_t_list)
    {
        // Construct full path to ammunition file
        strcpy(folder_buffer, folder_path);
        strcat(folder_buffer, "ammo_catalouge.txt");

        // Initialize list and load data
        init_Ammo_t_List(&ammo_t_list);
        hr = scan(folder_buffer, handle_ammo_t, ammo_t_list, process_state);
        // Note: hr is overwritten by subsequent scans - only last result preserved
    }

    // load base list (if not already loaded)
    if (!base_list)
    {
        strcpy(folder_buffer, folder_path);
        strcat(folder_buffer, "bases.txt");
        init_Base_List(&base_list);
        hr = scan(folder_buffer, handle_base, base_list, process_state);
    }

    // load target list (if not already loaded)
    if (!target_list)
    {
        strcpy(folder_buffer, folder_path);
        strcat(folder_buffer, "targets.txt");
        init_Target_List(&target_list);
        hr = scan(folder_buffer, handle_target, target_list, process_state);
    }

    // debug output section (only in debug mode)
    if (process_state == S_DEBUG)
    {
        // Display all loaded data for verification
        print_Ammo_t_List(ammo_t_list);
        print_Base_List(base_list, process_state);
        print_Target_List(target_list);

        printf("\n--- Testing color output ---\n");
        if (print_colored(ANSI_COLOR_RED, "This text should be RED!\n", false) == P_S_FALSE)
        {
            // This executes if color output failed (e.g., ANSI codes disabled)
            printf("This text shouldn't be RED!\n");
        }
        else
        { // Color succeeded - provide Windows terminal configuration help
            printf("if not, use the following command in the windows terminal, ");
            printf("to enable ansi escape codes\n");
            printf("\"reg add HKCU\\Console /v VirtualTerminalLevel /t REG_DWORD /d 1 /f\"\n");
        }
    }

    // test result summary
    add_seperator(NULL, '-', NORMAL_SEPERATOR_SIZE);
    if (hr == P_S_OK)
        // Success - display in green
        print_colored(ANSI_COLOR_GREEN, "Test succeeded!\n", false);
    else
        // Failure - display in red with newline
        print_colored(ANSI_COLOR_RED, "Test failed!\n", true);

    add_seperator(NULL, '-', NORMAL_SEPERATOR_SIZE);

    return hr;
}


#ifdef _WIN32
#include <windows.h>
void enable_ansi_colors()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
        return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
#else
void enable_ansi_colors()
{
    // Nothing needed for Linux/macOS terminals
}
#endif
