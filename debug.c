#include "debug.h"
#include "error.h"
#include "load.h"
#include "color.h"
#include "commands.h"

#include <stdio.h>
#include <string.h>

HRESULT run_pre_test()
{
    printf("Starting test...\n");
    BaseList bases;
    init_base_list(&bases);

    HRESULT hr = S_OK;
    hr = scan("C:\\vscode\\ballistic\\bin\\test.txt", handle_base, &bases, S_DEBUG);
    printf("capacity: %d, items: %d\n", bases.capacity, bases.count);
    for (int i = 0; i < bases.count; i++)
    {
        printf("%d %s\n", bases.items[i].info.id, bases.items[i].info.label);
    }
    free_base_list(&bases);

    printf(ANSI_COLOR_RED "This text is RED!" ANSI_COLOR_RESET "\n");

    printf("----------------------------------\n");
    if (hr == S_OK)
        printf("Test succeeded!\n");
    else
        printf("Test failed!\n");
    printf("----------------------------------\n");

    return hr;
}

