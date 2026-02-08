#include "load.h"
#include "simulation\calculations.h"
#include "error.h"
#include "command\commands.h"
#include "helpers\color.h"
#include "helpers\helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <stdatomic.h>

#include "thread\worker_platform.h"
#include "os\input_poll.h"

// Global variable storing the working directory path for all data files
// Should be initialized via config file and used for all file operations
char *folder_path = NULL;

int prompt_save = 1;

// sets an upper limit for the number of parameters,
// so if the user gives an unfinished definiton, after 15 iterations the loop exits
#define Param_Check_Limit 15

/**
 * @brief Sets the working directory path for data files
 *
 * Allocates memory and copies the provided path, freeing any previously
 * allocated path. Ensures proper memory management when changing paths.
 *
 * @param new_path The new directory path to set (will be copied)
 *
 * @note The path should end with a directory separator
 * @warning Does not validate if the path exists or is accessible
 */
void set_folder_path(const char *new_path)
{
    if (!new_path)
        return;

    // Free old memory if already allocated
    free(folder_path);

    // Allocate new memory for the new path
    folder_path = malloc(strlen(new_path) + 1);
    if (!folder_path)
    {
        fprintf(stderr, "Memory allocation failed for folder_path\n");
        return;
    }

    strcpy(folder_path, new_path);
}

/**
 * @brief Frees the folder_path memory and resets the pointer
 *
 * Safely deallocates the global folder_path variable and sets it to NULL
 */
void free_folder_path(void)
{
    free(folder_path);
    folder_path = NULL;
}

/*The scan function is a legacy function, it was one of the first
input handling function that I wrote for this program.
I know that it could be better, and my newer praser functions are much cleaner.
The only reason it's was left in the program, to show my development*/

/**
 * @brief Scans and parses a data file with comment support
 *
 * Parses files containing records separated by semicolons, with support for
 * both line comments (//) and block comments. Each complete record
 * is passed to a handler function for processing.
 *
 * @param filename Path to the file to scan
 * @param handler Function pointer to process each parsed record
 * @param data User data passed to the handler (typically a linked list)
 * @param mode Process mode (S_DEBUG for verbose output, S_RUNNING for normal)
 * @return P_HRESULT P_S_OK on success, error code on failure
 *
 * @note Records must end with semicolon (;)
 * @note Supports nested comments and error reporting for partial records
 * @see ParseState enum for parsing states
 */
P_HRESULT scan(const char *filename, RecordHandler handler, void *data, int *err_cnt, PROCESS mode)
{
    FILE *file = fopen(filename, "r"); // "r" open file in read only mode
    if (!file)
    {
        Error_message("Failed to open or locate '"ANSI_BRIGHT_RED "%s" COLOR_ERROR"' in the working directory!\n", filename);
        return P_S_FALSE;
    }

    int c = fgetc(file);
    if (c == EOF)
    {
        Error_message("Failed to read, or empty '"ANSI_BRIGHT_RED "%s" COLOR_ERROR"'!\n", filename);
        return P_S_FALSE;
    }
    ungetc(c, file);

    ParseState state = NORMAL;
    char buffer[1024];
    int index = 0;
    P_HRESULT hr = P_S_OK; // handle to a result
    if (err_cnt)
        *err_cnt = 0;

    // Process file character by character with state machine
    while ((c = fgetc(file)) != EOF && SUCCEEDED(hr))
    {
        switch (state)
        {
        case NORMAL:
        {
            if (c == '/')
            {
                int value = peek(file);
                if (value == '/')
                    state = LINE_COMMENT;
                else if (value == '*')
                    state = BLOCK_COMMENT;
                else
                { // invalid line
                    fprintf(stderr, "Warning: invalid syntax, skipping line\n");
                    while ((c = fgetc(file)) != EOF && c != '\n')
                        ;
                }
            }
            else if (!isspace(c))
            {
                // start buffering a record
                ungetc(c, file);
                state = RECORD;
            }

            break;
        }
        case LINE_COMMENT:
        {
            if (mode == S_DEBUG)
                printf("[Line comment] : \n");
            while ((c = fgetc(file)) != EOF)
            {
                if (mode == S_DEBUG)
                    putchar((unsigned char)c);

                if (c == '\n')
                    break;
            }
            state = NORMAL;
            break;
        }
        case BLOCK_COMMENT:
        {
            if (mode == S_DEBUG)
                printf("[Block comment] : \n");
            while ((c = fgetc(file)) != EOF)
                if (c == '*' && peek(file) == '/')
                    break;
                else if (mode == S_DEBUG)
                    putchar(c);

            fgetc(file);
            state = NORMAL;
            break;
        }
        case RECORD:
        {
            if (c == ';')
            {
                // terminate buffer
                buffer[index] = '\0';
                int error_flags[5] = {0};
                // a 1024 sized array gets passed to the handler
                if (handler)
                    hr = handler(buffer, data, error_flags); // pass the buffer and the user input address to the function

                else
                {
                    printf("No input handler!\n");
                    hr = P_S_FALSE;
                }

                // Handle partial records (missing required parameters)
                if (hr == P_S_PARTIAL)
                {
                    char error[128] = {0};
                    if (format_basic_error_string(error, error_flags) == S_FALSE)
                        Error_message("Error buffer overflow!\n");
                    Error_message("Missing parameter in the file [%s] : \nat the line [%s] : \n suggested parameters to add [%s]\n", filename, buffer, error);
                }
                else if (err_cnt && hr == P_S_FALSE)
                    err_cnt++;

                index = 0;
                state = NORMAL;
            }
            else if (index < (int)sizeof(buffer) - 1) // update the buffer loader to not store useless spaces, only store 1
            {
                if (isspace(c))
                    state = NORMAL;
                buffer[index++] = (char)c;
            }
            else
            {
                Error_message("buffer overflow!");
                hr = P_S_FALSE;
            }

            break;
        }
        }
    }

    if (fclose(file) == EOF)
    {
        Error_message("Failed to close '" ANSI_BRIGHT_RED "%s" COLOR_ERROR "'!\n");
        hr = E_FAIL;
    }
    return hr;
}

/**
 * @brief Peeks at the next character in a file without consuming it
 *
 * Reads the next character from the file stream, then pushes it back
 * so it can be read again by the next fgetc() call.
 *
 * @param file FILE pointer to peek into
 * @return The next character, or EOF if at end of file
 *
 * @note Uses ungetc() which is guaranteed to work for at least one character
 */
int peek(FILE *file)
{
    int c = fgetc(file);
    if (c != EOF)
        ungetc(c, file); // put it back
    return c;
}

/**
 * @brief Formats missing parameter error messages from error flags
 *
 * Converts an array of error flags into a human-readable string indicating
 * which parameters are missing from a record.
 *
 * @param error_buffer Output buffer for the error message
 * @param error_flags Array of 5 flags indicating missing parameters
 * @return P_HRESULT P_S_OK on success, S_FALSE on buffer overflow
 *
 * @note Error flags correspond to: F_ID, F_LABEL, F_LATITUDE, F_LONGITUDE, F_ALTITUDE
 * @warning error_buffer must be large enough (128 bytes recommended)
 */
P_HRESULT format_basic_error_string(char *error_buffer, int *error_flags)
{
    int er_cnt = -1;

    for (int i = 0; i < 5; i++)
    {
        if (error_flags[i] != 1)
        {
            er_cnt++;
            // the i represent the flag array, which holds their value
            // after that, it's just basic string phrasing
            switch (i)
            {
            case F_ID:
            {
                const char *id = "I";
                if (er_cnt > 0)
                    id = ", I";
                strncat(error_buffer, id, sizeof(error_buffer) - strlen(error_buffer) - 1);
                continue;
            }
            case F_LABEL:
            {
                const char *id = "L";
                if (er_cnt > 0)
                    id = ", L";
                strncat(error_buffer, id, sizeof(error_buffer) - strlen(error_buffer) - 1);
                continue;
            }
            case F_LATITUDE:
            {
                const char *id = "CLA";
                if (er_cnt > 0)
                    id = ", CLA";
                strncat(error_buffer, id, sizeof(error_buffer) - strlen(error_buffer) - 1);
                continue;
            }
            case F_LONGITUDE:
            {
                const char *id = "CLO";
                if (er_cnt > 0)
                    id = ", CLO";
                strncat(error_buffer, id, sizeof(error_buffer) - strlen(error_buffer) - 1);
                continue;
            }
            case F_ALTITUDE:
            {
                const char *id = "CA";
                if (er_cnt > 0)
                    id = ", CA";
                strncat(error_buffer, id, sizeof(error_buffer) - strlen(error_buffer) - 1);
                continue;
            }
            }
            return P_S_FALSE;
        }
    }
    // the error_buffer string is finsihed up, with a null terminator, just to be sure
    strncat(error_buffer, "\0", sizeof(error_buffer) - strlen(error_buffer));
    return P_S_OK;
}

// ===========================================================================
// CONFIGURATION FILE FUNCTIONS
// ===========================================================================

/**
 * @brief Initializes configuration or creates new one if missing
 *
 * Checks for existing config file, loads it if found, or interactively
 * creates a new one if missing. This is typically called at program startup.
 *
 * @param config_file Name of the configuration file
 * @param mode Process mode (S_DEBUG for verbose output)
 * @return P_HRESULT P_S_OK on success, error code on failure
 *
 * @note Prompts user to create config if file doesn't exist
 * @note Sets global folder_path and color_mode based on config
 */
P_HRESULT test_and_run_config(const char *config_file, PROCESS mode)
{
    printf(ANSI_GRAY "Loading config...\n");
    P_HRESULT hr = P_S_OK;
    FILE *configfile = fopen(config_file, "r"); // "r" open file in read only mode
    if (!configfile)
    {
        printf(COLOR_WARNING "Failed to open or locate [%s] in the working directory!\n", config_file);
        printf("Do you want to create a new one? ");
        cmd_help_inline(NULL, CMD_C_YESNO, NULL);
        printf(ANSI_RESET "\n");

        switch (GetCommand(CMD_C_MODAL | CMD_C_YESNO, NULL))
        {
        case CMD_YES:
        {
            printf("Starting config creation...\n");
            hr = evaluate_generate_config(config_file);
            if (P_SUCCEEDED(hr))
            {
                goto ReadConfig;
            }

            break;
        }
        case CMD_NO:
        {
            Error_message("Config creation was interrupted!\n");
            hr = E_FAIL;
            break;
        }
        default:
            break;
        }
        printf(ANSI_RESET);
        return hr;
    }
    fclose(configfile);

ReadConfig:
    // if everything succeded, then it reads and loads the config
    hr = read_config(config_file);
    add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    if (SUCCEEDED(hr))
    {
        if (mode == S_DEBUG)
            printf(ANSI_GRAY "Working directory have been set to: [%s]\n", folder_path);
        printf(COLOR_SUCCESS "Config loaded succesfully!\n" COLOR_RESET);
    }
    else
        Error_message(" Failed to load config!\n");

    add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    printf(ANSI_RESET "\n");
    return hr;
}

/**
 * @brief Recreates configuration file with new settings
 *
 * Interactive function to regenerate the configuration file with new
 * working directory and display mode settings.
 *
 * @param config_file Path to the configuration file to recreate
 * @param mode Process mode (S_DEBUG for verbose output)
 * @return P_HRESULT P_S_OK on success, S_FALSE on failure
 *
 * @note Uses generate_config() to create new file, then reads it back
 */
P_HRESULT recreate_config(const char *config_file, PROCESS mode)
{
    P_HRESULT hr = P_S_OK;

    if (evaluate_generate_config(config_file) == P_S_FALSE)
        return P_S_FALSE;

    hr = read_config(config_file);
    if (SUCCEEDED(hr))
    {
        if (mode == S_DEBUG)
            printf("Working directory have been set to: [%s]\n", folder_path);
        add_seperator_s(NULL, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(COLOR_SUCCESS "Config updated successfully!\n");
        add_seperator_s(NULL, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf("\n");
    }
    return hr;
}

/**
 * @brief Interactive configuration file generator
 *
 * Guides user through creating a new configuration file by prompting for
 * working directory path and color display mode preferences.
 *
 * @param configfile FILE pointer (unused parameter, could be removed)
 * @param config_file Path where config file should be created
 * @return P_HRESULT P_S_OK on success, error code on failure
 *
 * @note Creates file with comments explaining format and parameters
 * @note Prompts for color mode (CM_COLOUR or CM_MONO)
 */
P_HRESULT generate_config(const char *config_file)
{
    const char *file_list[] = {
        "bases.txt",
        "targets.txt",
        "ammo_catalouge.txt"};

Entry:
    printf("Enter a new folder path or type ");
    cmd_help_inline(NULL, CMD_C_BACK, NULL);
    printf(" to exit:\n");

Entry_no_print:

    char temp[1024]; // temporary buffer for user input
    printf("> ");
    if (fgets(temp, sizeof(temp), stdin))
    {
        Command_t cmd = ParseCommandFromBuffer(temp, CMD_C_BACK | CMD_C_NOCOMMENT, NULL);
        if (cmd == CMD_BACK)
            return P_S_FALSE;
        if (cmd == CMD_HELP)
            goto Entry_no_print;

        temp[strcspn(temp, "\n")] = '\0'; // remove trailing newline

        ensure_trailing_backslash(temp);
        int valid_path = 1;
        for (size_t i = 0; i < SIZE_OF_STRING_ARRAY(file_list); i++)
        {
            Normalize_path(temp);
            if (validate_file_path("%s%s", temp, file_list[i]) == P_S_FALSE)
            {
                Error_message("Missing file '" ANSI_BRIGHT_RED "%s" COLOR_ERROR "' at '" ANSI_BRIGHT_RED "%s" COLOR_ERROR "'!\n", file_list[i], temp);
                valid_path = 0;
            }
        }
        if (!valid_path)
            goto Entry;
        set_folder_path(temp);
    }

    FILE *configfile = fopen(config_file, "w"); // create a new config file
    if (!config_file)
        return P_S_FALSE;

    printf("Enable save prompt on exit? ");
    cmd_help_inline(NULL, CMD_C_YESNO, NULL);
    putchar('\n');
    prompt_save = (GetCommand(CMD_C_MODAL | CMD_C_YESNO, NULL) == CMD_YES);

    // Write configuration file with helpful comments
    fprintf(configfile, "/*\n=====================================================\n");
    fprintf(configfile, "--------Parameters-------------------------------\n");
    fprintf(configfile, "working directory :\n");
    fprintf(configfile, "Display the option to save on exit : [0=leave without saving, 1=propmt save]\n");
    fprintf(configfile, "=====================================================\n");
    fprintf(configfile, "-the id gets normalized\n");
    fprintf(configfile, "-end every line with ;\n");
    fprintf(configfile, "=====================================================\n*/\n");
    fprintf(configfile, "\n");
    fprintf(configfile, "DIR: \"%s\";\n", folder_path);
    fprintf(configfile, "PROMPT_SAVE: %d;\n", prompt_save);

    if (fclose(configfile) != 0)
    {
        fprintf(stderr, "Config file creation error! Couldn't close config file!");
        return E_FAIL;
    }
    return P_S_OK;
}

P_HRESULT evaluate_generate_config(const char *config_file)
{
    P_HRESULT hr = generate_config(config_file);
    if (hr == P_S_OK)
    {
        printf(COLOR_SUCCESS "Config creation succeded!\n" COLOR_RESET);
        return hr;
    }
    Error_message("Config creation failed!\n");
    return hr;
}
/**
 * Reads and parses existing configuration file
 *
 * Opens and scans a configuration file, using handle_config() to process
 * each configuration directive.
 *
 * @param config_file Path to the configuration file
 * @return P_HRESULT P_S_OK on success, error code on failure
 *
 * @note Uses the generic scan() function with handle_config() handler
 * @note Sets global folder_path and color_mode based on parsed values
 */
P_HRESULT read_config(const char *config_file)
{
    FILE *configfile = fopen(config_file, "r");
    if (!configfile)
    {
        Error_message(" Failed to open or locate [%s] in the working directory!\n", config_file);
        return P_S_FALSE;
    }

    P_HRESULT hr = scan(config_file, handle_config, NULL, NULL, S_RUNNING);
    if (hr == E_FAIL)
        return E_FAIL;
    if (hr == P_S_FALSE)
    {
        printf("Recreate config? ");
        cmd_help_inline(NULL, CMD_C_YESNO, NULL);
        putchar('\n');
        if (GetCommand(CMD_C_YESNO | CMD_C_MODAL, NULL) == CMD_YES)
        {
            if (evaluate_generate_config(config_file) == P_S_FALSE)
                return E_FAIL;
        }
    }

    if (fclose(configfile) == EOF)
        return E_FAIL;

    return P_S_OK;
}

/**
 * Configuration file record handler
 *
 * Processes individual configuration directives from the config file.
 * Recognizes DIR (working directory) and COLOUR_M (color mode) directives.
 *
 * @param record The configuration record string to parse
 * @param data Unused parameter (required by RecordHandler signature)
 * @param error_flags Unused parameter (required by RecordHandler signature)
 * @return P_HRESULT P_S_OK on successful parsing
 *
 * @note Uses strtoken() and norm_strncmp() for parsing
 * @note Sets global variables: folder_path and color_mode
 */
P_HRESULT handle_config(char *record, void *data, int *error_flags)
{
    (void)data;
    (void)error_flags;
    char *end = NULL;

    // searches for the normlaized DIR token, then if it is, searches for the value after the ':' charcter
    if (norm_strncmp(strtoken(record, &end), "DIR") == P_S_OK && (record = strchr(end, ':')) != NULL)
    {
        set_folder_path(Normalize_path(ensure_trailing_backslash(remove_quotes((char *)strtoken(record, &record), '\"'))));
        return P_S_OK;
    }

    // searches for the normlaized COLOUR_M token, then if it is, searches for the value after  the ':' charcter
    if (norm_strncmp(strtoken(record, &end), "PROMPT_SAVE") == P_S_OK && (record = strchr(end, ':')) != NULL)
    {
        prompt_save = ((int)strtof(strtoken(record, &end), &record) > 0) ? 1 : 0;
        return P_S_OK;
    }

    return P_S_FALSE;
}

// ===========================================================================
// DATA FILE HANDLERS (Ammo, Base, Target)
// ===========================================================================

// helper for phrasing error formating
void print_phrase_fail(const char *record_cpy, char *record, char *end)
{
    Error_message("");
    printf(COLOR_ERROR "Invalid token '" ANSI_BRIGHT_RED ANSI_UNDERLINE);
    record = strtoken(record, &end);
    print_substring(record, end);
    printf(ANSI_RESET COLOR_ERROR "' in line '" ANSI_BRIGHT_RED "%s" COLOR_ERROR "'!\n" COLOR_RESET, record_cpy);
}
/**
 * @brief Ammunition type record handler
 *
 * Parses ammunition type records from data files. Each record contains
 * ammunition specifications like weight, diameter, muzzle velocity, etc.
 *
 * @param record The record string to parse (format: I:id N:name W:weight etc.)
 * @param data Pointer to ammunition type linked list
 * @param error_flags Unused parameter (required by RecordHandler signature)
 * @return P_HRESULT P_S_OK on successful parsing
 *
 * @note Parameters: I=ID, N=Name, W=Weight, D=Diameter, BR=BlastRadius,
 *       MV=MuzzleVelocity, BC=BallisticCoefficient
 * @note Allocates memory for name field which is freed after adding to list
 * @warning Has hard-coded limit of 15 parameter checks per record
 */
P_HRESULT handle_ammo_t(char *record, void *data, int *error_flags)
{
    (void)error_flags;
    // record in the scan case is only on the stack, so I can modify the pointer without a problem
    Ammo_t ammo;
    ammo.name = NULL;
    char *ptr_cpy = record;
    char *end = NULL;

    // hard cap, so if the user gives some invalid parameters, the program won't get stuck
    int i = 0;
    for (i = 0; i < Param_Check_Limit && strlen(record) > 0; i++)
    {
        end = NULL;

        /*id*/
        if (norm_strncmp(record, "I") == P_S_OK)
        {
            record = strtoken(record, &end);
            ammo.id = (unsigned)strtof(++record, &end);
            record = strtoken(end, &end);
        }
        /*name*/
        if (norm_strncmp(record, "L") == P_S_OK)
        {
            record = strtoken(record, &end);
            record++;
            ammo.name = (char *)malloc(end - record + 1); // +1 for null terminator
            strncpy(ammo.name, record, end - record);
            ammo.name[end - record] = '\0';

            record = strtoken(end, &end);
        }
        /*weight*/
        if (norm_strncmp(record, "W") == P_S_OK)
        {
            record = strtoken(record, &end);
            ammo.weight = strtod(++record, &end);
            record = strtoken(end, &end);
        }
        /*diameter*/
        if (norm_strncmp(record, "D") == P_S_OK)
        {
            record = strtoken(record, &end);
            ammo.diameter = strtod(++record, &end);
            record = strtoken(end, &end);
        }
        /*blast radius*/
        if (norm_strncmp(record, "BR") == P_S_OK)
        {
            record = strtoken(record, &end);
            record++;
            ammo.blastRadius = strtod(++record, &end);
            record = strtoken(end, &end);
        }

        /*muzzle velocity*/
        if (norm_strncmp(record, "MV") == P_S_OK)
        {
            record = strtoken(record, &end);
            record++;
            ammo.muzzleVelocity = strtod(++record, &end);
            record = strtoken(end, &end);
        }

        /*ballistic coefficent*/
        if (norm_strncmp(record, "BC") == P_S_OK)
        {
            record = strtoken(record, &end);
            record++;
            ammo.ballistic_coefficient = strtod(++record, &end);
            record = strtoken(end, &end);
        }
    }
    if (i == Param_Check_Limit)
    {
        print_phrase_fail(ptr_cpy, record, end);

        if (ammo.name)
            free(ammo.name);
        return P_S_FALSE;
    }
    add_Ammo_t(data, ammo);

    free(ammo.name);

    return P_S_OK;
}

/**
 * @brief Base record handler
 *
 * Parses base records from data files. Each record contains base information
 * including coordinates and ammunition stock inventory.
 *
 * @param record The record string to parse (format: I:id L:label CLA:lat etc.)
 * @param data Pointer to base linked list
 * @param error_flags Unused parameter (required by RecordHandler signature)
 * @return P_HRESULT P_S_OK on successful parsing
 *
 * @note Parameters: I=ID, L=Label, CLA=Latitude, CLO=Longitude, CA=Altitude,
 *       AMS=Ammunition Stock (list of ammo ID:amount pairs)
 * @note AMS format: AMS: (id amount) (id amount) ... ;
 * @note Allocates memory for label field which is freed after adding to list
 * @warning Has hard-coded limit of 15 parameter checks per record
 */
P_HRESULT handle_base(char *record, void *data, int *error_flags)
{
    (void)error_flags;

    // record in the scan case is only on the stack, so I can modify the pointer without a problem
    Base base;
    base.label = NULL;
    char *ptr_cpy = record;
    char *end = NULL;

    // hard cap, so if the user gives some invalid parameters, the program won't get stuck
    int i = 0;
    for (i = 0; i < Param_Check_Limit && strlen(record) > 0; i++)
    {
        end = NULL;

        // id
        if (norm_strncmp(record, "I") == P_S_OK)
        {
            record = strtoken(record, &end);
            base.id = (unsigned)strtof(++record, &end);
            record = strtoken(end, &end);
        }
        // name
        if (norm_strncmp(record, "L") == P_S_OK)
        {
            record = strtoken(record, &end);
            record++;
            base.label = (char *)malloc(end - record + 1); // +1 for null terminator
            strncpy(base.label, record, end - record);
            base.label[end - record] = '\0';

            record = strtoken(end, &end);
        }
        // latitude
        if (norm_strncmp(record, "CLA") == P_S_OK)
        {
            record = strtoken(record, &end);
            record += 3;
            base.coord.latitude = strtod(record, &end);
            record = strtoken(end, &end);
        }
        // longitude
        if (norm_strncmp(record, "CLO") == P_S_OK)
        {
            record = strtoken(record, &end);
            record += 3;
            base.coord.longitude = strtod(record, &end);
            record = strtoken(end, &end);
        }
        // altitude
        if (norm_strncmp(record, "CA") == P_S_OK)
        {
            record = strtoken(record, &end);
            record += 2;
            base.coord.altitude = strtod(record, &end);
            record = strtoken(end, &end);
        }
        // ammo list
        if (norm_strncmp(record, "AMS") == P_S_OK)
        {
            record += 3;    //length of "AMS"
            record = strtoken(record, &end);
            record = strtoken(++record, &end);

            base.ammoStock = NULL;
            init_Ammo_Stock_List(&base.ammoStock);

            // Parse ammunition stock list: (id amount) pairs
            while (strlen(record) > 1)
            {
                // add ammo type, to combed list
                unsigned id = (unsigned)strtof(record, &end);
                record = strtoken(end, &end);

                unsigned ammount = (unsigned)strtof(record, &end);
                record = strtoken(end, &end);
                record++;

                add_Ammo_Stock(base.ammoStock, GetNode_Ammo_t_List(ammo_t_list, id), ammount);
            }
        }
    }
    if (i == Param_Check_Limit || i == 0 || !record)
    {
        print_phrase_fail(ptr_cpy, record, end);

        if (base.label)
            free(base.label);
        return P_S_FALSE;
    }

    add_Base(data, base);
    free(base.label);

    return P_S_OK;
}

/**
 * @brief Target record handler
 *
 * Parses target records from data files. Each record contains target information
 * including coordinates and target area size.
 *
 * @param record The record string to parse (format: I:id L:label CLA:lat etc.)
 * @param data Pointer to target linked list
 * @param error_flags Unused parameter (required by RecordHandler signature)
 * @return P_HRESULT P_S_OK on successful parsing
 *
 * @note Parameters: I=ID, L=Label, CLA=Latitude, CLO=Longitude, CA=Altitude, A=Area
 * @note Allocates memory for label field which is freed after adding to list
 * @warning Has hard-coded limit of 15 parameter checks per record
 */
P_HRESULT handle_target(char *record, void *data, int *error_flags)
{
    (void)error_flags;

    // record in the scan case is only on the stack, so I can modify the pointer without a problem
    Target target;
    target.label = NULL;

    char *ptr_cpy = record;
    char *end = NULL;

    // hard cap, so if the user gives some invalid parameters, the program won't get stuck
    int i = 0;
    for (i = 0; i < Param_Check_Limit && strlen(record) > 0; i++)
    {
        end = NULL;

        // id
        if (norm_strncmp(record, "I") == P_S_OK)
        {
            record = strtoken(record, &end);
            target.id = (unsigned)strtof(++record, &end);
            record = strtoken(end, &end);
        }
        // name
        if (norm_strncmp(record, "L") == P_S_OK)
        {
            record = strtoken(record, &end);
            record++;
            target.label = (char *)malloc(end - record + 1); // +1 for null terminator
            strncpy(target.label, record, end - record);
            target.label[end - record] = '\0';

            record = strtoken(end, &end);
        }
        // latitude
        if (norm_strncmp(record, "CLA") == P_S_OK)
        {
            record = strtoken(record, &end);
            record += 3;
            target.coord.latitude = strtod(record, &end);
            record = strtoken(end, &end);
        }
        // longitude
        if (norm_strncmp(record, "CLO") == P_S_OK)
        {
            record = strtoken(record, &end);
            record += 3;
            target.coord.longitude = strtod(record, &end);
            record = strtoken(end, &end);
        }
        // altitude
        if (norm_strncmp(record, "CA") == P_S_OK)
        {
            record = strtoken(record, &end);
            record += 2;
            target.coord.altitude = strtod(record, &end);
            record = strtoken(end, &end);
        }
        // area
        if (norm_strncmp(record, "A") == P_S_OK)
        {
            record = strtoken(record, &end);
            target.area = strtod(++record, &end);
            record = strtoken(end, &end);
        }
    }
    if (i == Param_Check_Limit)
    {
        print_phrase_fail(ptr_cpy, record, end);

        if (target.label)
            free(target.label);
        return P_S_FALSE;
    }

    add_Target(data, target);

    free(target.label);

    return P_S_OK;
}

// ===========================================================================
// LOGGING AND USER INTERACTION
// ===========================================================================

/**
 * @brief Logs calculation results to file
 *
 * Appends a detailed log entry to log.txt in the working directory,
 * containing all parameters and results of a trajectory calculation.
 *
 * @param base Pointer to base used in calculation
 * @param tgt Pointer to target used in calculation
 * @param ammo Pointer to ammunition type used in calculation
 * @param res Pointer to ballistic calculation results
 *
 * @note Includes timestamp, all parameters, and calculation results
 * @note Log file is appended to, not overwritten
 * @warning All pointers must be valid (non-NULL)
 */
void LogResult(const Base *base, const Target *tgt, const Ammo_t *ammo, const BallisticResult *res)
{
    if (!base || !tgt || !ammo || !res)
    {
        Error_message(" LogResult: invalid argument.\n");
        return;
    }

    char folder_buffer[1024];
    strcpy(folder_buffer, folder_path);
    strcat(folder_buffer, "log.txt");
    FILE *fp = fopen(folder_buffer, "a");
    if (!fp)
    {
        Error_message("Failed to open log.txt for writing.\n");
        return;
    }

    // get the time for log
    time_t now = time(NULL);
    char *ts = ctime(&now);
    if (ts)
        ts[strcspn(ts, "\n")] = 0; // strip newline

    fprintf(fp,
            "=== ENTRY @ %s ===\n"
            "Base:\n"
            "  ID: %u\n"
            "  Label: %s\n"
            "  Coord: latitude %.6f, longitude %.6f, altitude %.2f m\n"
            "\n"
            "Target:\n"
            "  ID: %u\n"
            "  Label: %s\n"
            "  Coord: latitude %.6f, longitude %.6f, altitude %.2f m\n"
            "\n"
            "Ammo:\n"
            "  ID: %u\n"
            "  Name: %s\n"
            "  Weight: %.3f kg\n"
            "  Diameter: %.3f mm\n"
            "  BlastRadius: %.3f m\n"
            "  MuzzleVel: %.3f m/s\n"
            "  BC(G7): %.5f\n"
            "\n"
            "Solution:\n"
            "  Elevation: %.4f deg\n"
            "  Azimuth:   %.4f deg\n"
            "  TOF:       %.4f s\n"
            "  ImpactVel: %.4f m/s\n"
            "  Valid:     %s\n"
            "-----------------------------\n\n",

            ts,
            base->id, base->label,
            base->coord.latitude, base->coord.longitude, base->coord.altitude,

            tgt->id, tgt->label,
            tgt->coord.latitude, tgt->coord.longitude, tgt->coord.altitude,

            ammo->id, ammo->name,
            ammo->weight,
            ammo->diameter,
            ammo->blastRadius,
            ammo->muzzleVelocity,
            ammo->ballistic_coefficient,

            res->elevation_deg,
            res->azimuth_deg,
            res->time_of_flight,
            res->impact_velocity,
            res->valid ? "YES" : "NO");

    fclose(fp);
}

/**
 * @brief Interactive parameter selection for calculations
 *
 * Guides user through selecting base, target, and ammunition for trajectory
 * calculations. Automatically suggests alternatives if selected combination
 * is invalid or unavailable.
 *
 * @return int 0 on successful selection, -1 to return to previous menu
 *
 * @note Validates IDs and checks if base can hit target with selected ammo
 * @note Uses SuggestBestBaseAndAmmo_RegionAware() to find alternatives
 * @note Logs successful selections to file via LogResult()
 */
int HandleParameterSelection(void)
{
    unsigned baseID, targetID, ammoID;
    char input[128] = {0};
    add_seperator_c(NULL, NULL, NULL, '-', NORMAL_SEPERATOR_SIZE);
    printf(" Enter parameters: <base_id> <target_id> <ammo_id>\n");
    printf(" Type [back] to return or to view/expand the available [list]!\n");
    add_seperator_c(NULL, NULL, NULL, '-', NORMAL_SEPERATOR_SIZE);
    printf("\n");

    switch (back_request(input, sizeof(input)))
    {
    case 1:
        return -1;
    case 0:
        break;
    case -1:
        return 0;
    default:
        return 0;
    }

    // Parse numerical IDs
    if (sscanf(input, "%u %u %u", &baseID, &targetID, &ammoID) != 3)
    {
        printf(COLOR_ERROR " Invalid input. Expected format: <base_id> <target_id> <ammo_id>\n" ANSI_RESET);
        printf(" Press any key to return!\n");
        while (!input_available())
            ;
        input_getchar();
        clear_terminal();
        return -1;
    }

    Base *base = FindBaseByID(base_list, baseID);
    Target *tgt = FindTargetByID(target_list, targetID);
    Ammo_t *ammo = FindAmmoByID(ammo_t_list, ammoID);

    if (!tgt)
    {
        printf("Target ID %u does not exist.\n", targetID);
        return 0;
    }

    // If base or ammo is invalid or cannot hit target, suggest automatically
    BallisticResult b_result;
    FireCheckResult c_result;
    if (!base || !ammo || ((c_result = test_fire_solution(base, tgt, ammo, &b_result, NULL)) != FIRE_OK))
    {
        clear_terminal();
        printf(COLOR_ERROR " Invalid input");
        switch (c_result)
        {
        case FIRE_TOO_CLOSE:
        {
            printf(COLOR_ERROR ", base is too close to the target!\n");
            break;
        }
        case FIRE_NO_AMMO:
        {
            printf(COLOR_ERROR ", base has no ammo type!\n");
            break;
        }
        case FIRE_OUT_OF_RANGE:
        {
            printf(COLOR_ERROR ", base is too far from the target!\n");
            break;
        }
        case FIRE_INVALID_INPUT:
        {
            printf(COLOR_ERROR ", missing parameter!\n");
            break;
        }
        default:
            printf("!\n");
            break;
        }
        printf(ANSI_RESET);

        printf(" Suggest base and ammo? ");
        cmd_help_inline(NULL, CMD_C_YESNO, NULL);
        if (GetCommand(CMD_C_MODAL | CMD_C_YESNO, NULL) == CMD_NO)
            return 0;

        printf(" Finding base and ammo for [id: %3d | label: %10s]\n", tgt->id, tgt->label);
        printf(" This may take a while...\n");
        printf(" Press [q] or [Esc] to terminate the calculation!\n");
        printf(COLOR_HINT ASCI_ANGEL COLOR_RESET);
        // the entry for the calculation model loop for multi threading

        if (!SuggestBestBaseAndAmmo_Parallel(base_list, tgt, &base, &ammo))
        {
            printf("No suitable base/ammo combination found.\n");
            return 0;
        }
        add_seperator_c(NULL, NULL, NULL, '-', NORMAL_SEPERATOR_SIZE);
        printf("Suggested base and ammo for [%u | %s]:\n", tgt->id, tgt->label);
        printf("  Base ID: %u  Label: %s  Coordinates: lat %.6f, lon %.6f, alt %.2f m\n",
               base->id, base->label, base->coord.latitude, base->coord.longitude, base->coord.altitude);
        printf("  Ammo ID: %u  Label: %s\n", ammo->id, ammo->name);
        add_seperator_c(NULL, NULL, NULL, '-', NORMAL_SEPERATOR_SIZE);
        printf("\n");

        printf("Would you like to accept these changes? ");
        cmd_help_inline(NULL, CMD_C_YESNO, NULL);
        printf("\n");
        if (GetCommand(CMD_C_MODAL | CMD_C_YESNO, NULL) == CMD_YES)
        {
            goto Log;
        }

        return 0;
    }

Log:
    LogResult(base, tgt, ammo, &b_result);
    printf("Parameters accepted and logged.\n");
    return 0;
}

unsigned get_base_ammo_t_count(const Base *base)
{
    if (!base || !base->ammoStock)
        return 0;
    unsigned total = 0;

    for (Ammo_Stock *s = base->ammoStock->head; s; s = s->next)
    {
        if (s->typePtr && s->quantity > 0)
            total++;
    }

    return total;
}

/**
 * @brief Validates a file path
 *
 * @return P_S_OK if it exists, P_S_FALSE if not
 *
 * @param const char *folder_path: the files folder path
 * @param ... the file name to validate
 */
P_HRESULT validate_file_path(const char *folder_path, ...)
{
    char buffer[1024];
    FILE *fp = NULL;
    va_list ap;
    va_start(ap, folder_path);
    vsprintf(buffer, folder_path, ap);

    fp = fopen(buffer, "r");
    if (!fp)
        return P_S_FALSE;

    fclose(fp);
    return P_S_OK;
}