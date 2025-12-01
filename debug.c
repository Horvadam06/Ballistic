#include "debug.h"
#include "error.h"
#include "load.h"
#include "color.h"
#include "commands.h"

#include <stdio.h>
#include <string.h>

#include "debugmalloc.h"

HRESULT run_pre_test(PROCESS process_state)
{
    printf("Starting test...\n");

    HRESULT hr = S_OK;

    char folder_buffer[1024];
    strcpy(folder_buffer, folder_path);
    strcat(folder_buffer, "ammo_catalouge.txt");
    init_Ammo_t_List(&ammo_t_list);
    hr = scan(folder_buffer, handle_ammo_t, ammo_t_list, process_state);

    strcpy(folder_buffer, folder_path);
    strcat(folder_buffer, "bases.txt");

    init_Base_List(&base_list);
    hr = scan(folder_buffer, handle_base, base_list, process_state);
    if (process_state == S_DEBUG)
    {
        print_Base_List(base_list, process_state);
        print_Ammo_t_List(ammo_t_list);

        if (print_colored(ANSI_COLOR_RED, "This text should be RED!\n", false) == S_FALSE)
        {
            printf("This text shouldn't be RED!\n");
        }
        else
            printf("if not, use the following command in the windows terminal, to enable ansi escape codes\n\"reg add HKCU\\Console /v VirtualTerminalLevel /t REG_DWORD /d 1 /f\"\n");
    }
    free_Base_List(&base_list);
    free_Ammo_t_List(&ammo_t_list);

    printf("----------------------------------\n");
    if (hr == S_OK)
        print_colored(ANSI_COLOR_GREEN, "Test succeeded!\n", false);
    else
        print_colored(ANSI_COLOR_RED, "Test failed!\n", true);
    printf("----------------------------------\n");

    return hr;
}
