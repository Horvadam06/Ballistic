#include <stdio.h>

#include "main.h"
#include "debug.h"
#include "commands.h"
#include "calculations.h"
#include "load.h"
#include "error.h"
#include "storage.h"

//=========================================
// Created by.:
// Starting date: 2025.10.17
// Finishing date:
//=========================================

PROCESS Setup()
{
    if (!SUCCEEDED(run_pre_test()))
        return S_TERMINATE;

    printf("\n");
    printf("==================================\n");
    printf("Ballistic trajectory calculator\n");
    printf("Created by.: Adam Jozsef Horvath\n");
    printf("==================================\n");
    printf("\n");
    printf("Type [exit] to leave or type [help] for the commands!\n");

    return S_RUNNING;
}

int main(void)
{
    PROCESS state = Setup();
    while (state == S_RUNNING)
    {
        printf("> ");
        GetCommand(&state);
    }
    if (state == S_END)
        printf("Program exited succesfully!\n");
    else if (state == S_TERMINATE)
        printf("Program terminated!\n");
    return 0;
}