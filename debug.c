#include "debug.h"
#include "error.h"
#include "load.h"
#include "color.h"
#include "commands.h"

#include <stdio.h>
#include <string.h>

HRESULT run_pre_test(PROCESS process_state)
{
    printf("Starting test...\n");
    BaseList bases;
    init_base_list(&bases);

    HRESULT hr = S_OK;
    hr = scan("C:\\vscode\\ballistic\\bin\\test.txt", handle_base, &bases, process_state);
    if (process_state == S_DEBUG)
    {
        printf("capacity: %d, items: %d\n", bases.capacity, bases.count);
        for (int i = 0; i < bases.count; i++)
        {
            printf("%d %s\n", bases.items[i].info.id, bases.items[i].info.label);
        }

        if (print_colored(ANSI_COLOR_RED, "This text should be RED!\n", false) == S_FALSE)
        {
            printf("This text shouldn't be RED!\n");
        }
    }
    free_base_list(&bases);

    printf("----------------------------------\n");
    if (hr == S_OK)
        print_colored(ANSI_COLOR_GREEN, "Test succeeded!\n", false);
    else
        print_colored(ANSI_COLOR_RED, "Test failed!\n", true);
    printf("----------------------------------\n");

    return hr;
}
