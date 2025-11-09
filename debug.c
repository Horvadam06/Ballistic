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

HRESULT first_run_test(const char *config_file)
{
    HRESULT hr = S_OK;
    FILE *configfile = fopen(config_file, "r"); // "r" open file in read only mode
    if (!configfile)
    {
        printf("Failed to open or locate [%s] in the working directory!\n", config_file);
        printf("Do you want to create a new one?\n");

        switch (GetCommand(CMD_C_LOOP | CMD_C_YESNO, NULL))
        {
        case CMD_YES:
        {
            char temp[1024]; // temporary buffer for user input
            printf("Enter a folder path:\n");
            /*
            after reading the length of the data, for example 10 bytes, it leaves the rest there,
            which cause the next command read to read in a false value
            if (scanf("%1023s", temp) == 1) // read at most 1023 chars, prevent overflow
                set_folder_path(temp);      // pass directly to your dynamic setter
            */
            if (fgets(temp, sizeof(temp), stdin))
            {
                temp[strcspn(temp, "\n")] = '\0'; // remove trailing newline
                set_folder_path(temp);
            }

            configfile = fopen(config_file, "w"); // create a new config file
            if (!config_file)
                return S_FALSE;

            printf("Enter the colour mode:\n");

            switch (GetCommand(CMD_C_LOOP | CMD_C_SETUP, NULL))
            {
            case CMD_COLOUR:
                strcpy(temp, "colour");
                break;
            case CMD_MONO:
                strcpy(temp, "monochrome");
                break;
            }

            fprintf(configfile, "=====================================================\n");
            fprintf(configfile, "--------Parameters-------------------------------\n");
            fprintf(configfile, "working directory []\n");
            fprintf(configfile, "color mode []\n");
            fprintf(configfile, "=====================================================\n");
            fprintf(configfile, "-the id gets normalized\n");
            fprintf(configfile, "-end every line with ;\n");
            fprintf(configfile, "=====================================================\n");
            fprintf(configfile, "\n");
            fprintf(configfile, "DIR [%s];\n", folder_path);
            fprintf(configfile, "COLOURM [%s];\n", temp);

            fclose(configfile);
            break;
        }
        case CMD_NO:
        {
            hr = E_FAIL;
            break;
        }
        }
    }

    configfile = fopen(config_file, "r");
    if (!configfile)
    {
        printf("Failed to open or locate [%s] in the working directory!\n", config_file);
        return S_FALSE;
    }

    if (fclose(configfile) == EOF)
        hr = E_FAIL;
    return hr;
}