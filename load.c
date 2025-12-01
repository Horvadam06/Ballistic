#include "load.h"
#include "calculations.h"
#include "error.h"
#include "commands.h"
#include "color.h"
#include "helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "debugmalloc.h"

// add the save to file option, if it's done, only the calculations left

char *folder_path = NULL;

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

void free_folder_path(void)
{
    free(folder_path);
    folder_path = NULL;
}

/*file name to scan  |  handler function  |  external data for the handler  |  scan mode*/
HRESULT scan(const char *filename, RecordHandler handler, void *data, PROCESS mode)
{
    FILE *file = fopen(filename, "r"); // "r" open file in read only mode
    if (!file)
    {
        printf("Failed to open or locate [%s] in the working directory!\n", filename);
        return S_FALSE;
    }

    int c;
    ParseState state = NORMAL;
    char buffer[512];
    int index = 0;
    HRESULT hr = S_OK; // handle to a result

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
                char idk = (char)c;
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
                // a 256 sized array gets passed to the handler, so you can safely step it's ptr
                if (handler)
                    hr = handler(buffer, data, error_flags); // pass the buffer and the user input address to the function

                else
                {
                    printf("No input handler!\n");
                    hr = S_FALSE;
                }

                if (hr == S_PARTIAL)
                {
                    char error[128] = {0};
                    if (format_basic_error_string(error, error_flags) == S_FALSE)
                        printf("[Error] : Error buffer overflow!\n");
                    printf("[Warning] : Missing parameter in the file [%s] : \nat the line [%s] : \n suggested parameters to add [%s]\n", filename, buffer, error);
                }
                index = 0;
                state = NORMAL;
            }
            else if (index < sizeof(buffer) - 1) // update the buffer loader to not store useless spaces, only store 1
            {
                if (isspace(c))
                    state = NORMAL;
                buffer[index++] = (char)c;
            }
            else
            {
                printf("[Error] buffer overflow!");
                hr = S_FALSE;
            }

            break;
        }
        }
    }

    if (fclose(file) == EOF)
        hr = E_FAIL;
    return hr;
}

int peek(FILE *file)
{
    int c = fgetc(file);
    if (c != EOF)
        ungetc(c, file); // put it back
    return c;
}

HRESULT format_basic_error_string(char *error_buffer, int *error_flags)
{
    int er_cnt = -1;

    for (int i = 0; i < 5; i++)
    {
        if (error_flags[i] != 1)
        {
            er_cnt++;
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
            return S_FALSE;
        }
    }
    strncat(error_buffer, "\0", sizeof(error_buffer) - strlen(error_buffer));
    return S_OK;
}

// config===========================================

HRESULT test_and_run_config(const char *config_file, PROCESS mode) // check if it's the first time, that the program is being ran, or if there is any missing info
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
            hr = generate_config(configfile, config_file);
            break;
        }
        case CMD_NO:
        {
            hr = E_FAIL;
            break;
        }
        }
        return hr;
    }
    fclose(configfile);

    hr = read_config(config_file);
    if (SUCCEEDED(hr))
    {
        if (mode == S_DEBUG)
            printf("Working directory have been set to: [%s]\n", folder_path);
        printf("----------------------------------\n");
        printf("Config loaded succesfully!\n");
        printf("----------------------------------\n");
        printf("\n");
    }
    return hr;
}

HRESULT recreate_config(const char *config_file, PROCESS mode)
{
    HRESULT hr = S_OK;
    FILE *configfile = NULL; // "r" open file in read only mode

    return generate_config(configfile, config_file);

    hr = read_config(config_file);
    if (SUCCEEDED(hr))
    {
        if (mode == S_DEBUG)
            printf("Working directory have been set to: [%s]\n", folder_path);
        printf("----------------------------------\n");
        printf("Config updated successfully!\n");
        printf("----------------------------------\n");
        printf("\n");
    }
    return hr;
}

HRESULT generate_config(FILE *configfile, const char *config_file)
{
    printf("Enter a folder path:\n");

    char temp[1024]; // temporary buffer for user input
    if (fgets(temp, sizeof(temp), stdin))
    {
        temp[strcspn(temp, "\n")] = '\0'; // remove trailing newline

        ensure_trailing_backslash(temp);
        Normalize_path(temp);

        set_folder_path(temp);
    }

    configfile = fopen(config_file, "w"); // create a new config file
    if (!config_file)
        return S_FALSE;

    printf("Enter the colour mode:\n");

    switch (GetCommand(CMD_C_LOOP | CMD_C_SETUP, NULL))
    {
    case CMD_COLOUR:
        set_color_mode(CM_COLOUR);
        break;
    case CMD_MONO:
        set_color_mode(CM_MONO);
        break;
    }

    fprintf(configfile, "/*\n=====================================================\n");
    fprintf(configfile, "--------Parameters-------------------------------\n");
    fprintf(configfile, "working directory :\n");
    fprintf(configfile, "color mode : [0=monochrome, 1=coloured]\n");
    fprintf(configfile, "=====================================================\n");
    fprintf(configfile, "-the id gets normalized\n");
    fprintf(configfile, "-end every line with ;\n");
    fprintf(configfile, "=====================================================\n*/\n");
    fprintf(configfile, "\n");
    fprintf(configfile, "DIR: \"%s\";\n", folder_path);
    fprintf(configfile, "COLOUR_M: %d;\n", get_color_mode());

    if (fclose(configfile) != 0)
    {
        printf("\n=========================\n");
        printf("[ERROR] Config file creation error");
        printf("\n=========================\n");
        return S_TERMINATE;
    }
    return S_OK;
}

HRESULT read_config(const char *config_file)
{

    FILE *configfile = fopen(config_file, "r");
    if (!configfile)
    {
        printf("Failed to open or locate [%s] in the working directory!\n", config_file);
        return S_FALSE;
    }

    scan(config_file, handle_config, NULL, S_RUNNING);

    if (fclose(configfile) == EOF)

        return E_FAIL;

    return S_OK;
}

HRESULT handle_config(char *record, void *data, int *error_flags)
{
    char *end = NULL;
    /*searches for the normlaized DIR token, then if it is, searches for the value after the ':' charcter*/
    if (norm_strncmp(strtoken(record, &end), "DIR") == S_OK && (record = strchr(end, ':')) != NULL)
    {
        set_folder_path(Normalize_path(ensure_trailing_backslash(remove_quotes((char *)strtoken(record, &record), '\"'))));
        return S_OK;
    }

    /*searches for the normlaized COLOUR_M token, then if it is, searches for the value after  the ':' charcter*/
    if (norm_strncmp(strtoken(record, &end), "COLOUR_M") == S_OK && (record = strchr(end, ':')) != NULL)
    {
        set_color_mode((int)strtof(strtoken(record, &end), &record));
        return S_OK;
    }
    return S_OK;
}

HRESULT handle_ammo_t(char *record, void *data, int *error_flags)
{
    // record in the scan case is only on the stack, so I can modify the pointer without a problem
    Ammo_t ammo;

    // hard cap, so if the user gives some invalid parameters, the program won't get stuck
    for (int i = 0; i < 15 && strlen(record) > 0; i++)
    {

        char *end = NULL;
        /*id*/
        if (norm_strncmp(record, "I") == S_OK)
        {
            record = strtoken(record, &end);
            ammo.id = (unsigned)strtof(++record, &end);
            record = strtoken(end, &end);
        }
        /*name*/
        if (norm_strncmp(record, "N") == S_OK)
        {
            record = strtoken(record, &end);
            record++;
            ammo.name = (char *)malloc(end - record + 1); // +1 for null terminator
            strncpy(ammo.name, record, end - record);
            ammo.name[end - record] = '\0';

            record = strtoken(end, &end);
        }
        /*weight*/
        if (norm_strncmp(record, "W") == S_OK)
        {
            record = strtoken(record, &end);
            ammo.weight = strtod(++record, &end);
            record = strtoken(end, &end);
        }
        /*diameter*/
        if (norm_strncmp(record, "D") == S_OK)
        {
            record = strtoken(record, &end);
            ammo.diameter = strtod(++record, &end);
            record = strtoken(end, &end);
        }
        /*blast radius*/
        if (norm_strncmp(record, "BR") == S_OK)
        {
            record = strtoken(record, &end);
            record++;
            ammo.blastRadius = strtod(++record, &end);
            record = strtoken(end, &end);
        }

        /*blast radius*/
        if (norm_strncmp(record, "MV") == S_OK)
        {
            record = strtoken(record, &end);
            record++;
            ammo.muzzleVelocity = strtod(++record, &end);
            record = strtoken(end, &end);
        }

        /*ballistic coefficent*/
        if (norm_strncmp(record, "BC") == S_OK)
        {
            record = strtoken(record, &end);
            record++;
            ammo.ballistic_coefficient = strtod(++record, &end);
            record = strtoken(end, &end);
        }
    }

    add_Ammo_t(data, ammo);

    free(ammo.name);

    return S_OK;
}

HRESULT handle_base(char *record, void *data, int *error_flags)
{
    // record in the scan case is only on the stack, so I can modify the pointer without a problem
    Base base;

    // hard cap, so if the user gives some invalid parameters, the program won't get stuck
    for (int i = 0; i < 15 && strlen(record) > 0; i++)
    {

        char *end = NULL;
        // id
        if (norm_strncmp(record, "I") == S_OK)
        {
            record = strtoken(record, &end);
            base.id = (unsigned)strtof(++record, &end);
            record = strtoken(end, &end);
        }
        // name
        if (norm_strncmp(record, "L") == S_OK)
        {
            record = strtoken(record, &end);
            record++;
            base.label = (char *)malloc(end - record + 1); // +1 for null terminator
            strncpy(base.label, record, end - record);
            base.label[end - record] = '\0';

            record = strtoken(end, &end);
        }
        // latitude
        if (norm_strncmp(record, "CLA") == S_OK)
        {
            record = strtoken(record, &end);
            record += 3;
            base.coord.latitude = strtod(record, &end);
            record = strtoken(end, &end);
        }
        // longitude
        if (norm_strncmp(record, "CLO") == S_OK)
        {
            record = strtoken(record, &end);
            record += 3;
            base.coord.longitude = strtod(record, &end);
            record = strtoken(end, &end);
        }
        // altitude
        if (norm_strncmp(record, "CA") == S_OK)
        {
            record = strtoken(record, &end);
            record += 2;
            base.coord.altitude = strtod(record, &end);
            record = strtoken(end, &end);
        }
        // ammo list
        if (norm_strncmp(record, "AMS") == S_OK)
        {
            record += 3;
            record = strtoken(record, &end);
            record = strtoken(++record, &end);

            base.ammoStock = NULL;
            init_Ammo_Stock_List(&base.ammoStock);

            while (strlen(record) > 1)
            {
                // add ammo type, to combed list
                unsigned id = (unsigned)strtof(record, &end);
                record = strtoken(end, &end);

                unsigned ammount = (unsigned)strtof(record, &end);
                record = strtoken(end, &end);
                record++;

                add_Ammo_Stock(base.ammoStock, GetNode_Ammot_t_List(ammo_t_list, id), ammount);
            }
        }
    }

    add_Base(data, base);

    free(base.label);

    return S_OK;
}

HRESULT handle_target(char *record, void *data, int *error_flags)
{
    // record in the scan case is only on the stack, so I can modify the pointer without a problem
    Target target;

    // hard cap, so if the user gives some invalid parameters, the program won't get stuck
    for (int i = 0; i < 15 && strlen(record) > 0; i++)
    {

        char *end = NULL;
        // id
        if (norm_strncmp(record, "I") == S_OK)
        {
            record = strtoken(record, &end);
            target.id = (unsigned)strtof(++record, &end);
            record = strtoken(end, &end);
        }
        // name
        if (norm_strncmp(record, "L") == S_OK)
        {
            record = strtoken(record, &end);
            record++;
            target.label = (char *)malloc(end - record + 1); // +1 for null terminator
            strncpy(target.label, record, end - record);
            target.label[end - record] = '\0';

            record = strtoken(end, &end);
        }
        // latitude
        if (norm_strncmp(record, "CLA") == S_OK)
        {
            record = strtoken(record, &end);
            record += 3;
            target.coord.latitude = strtod(record, &end);
            record = strtoken(end, &end);
        }
        // longitude
        if (norm_strncmp(record, "CLO") == S_OK)
        {
            record = strtoken(record, &end);
            record += 3;
            target.coord.longitude = strtod(record, &end);
            record = strtoken(end, &end);
        }
        // altitude
        if (norm_strncmp(record, "CA") == S_OK)
        {
            record = strtoken(record, &end);
            record += 2;
            target.coord.altitude = strtod(record, &end);
            record = strtoken(end, &end);
        }
        // area
        if (norm_strncmp(record, "A") == S_OK)
        {
            record = strtoken(record, &end);
            target.area = strtod(++record, &end);
            record = strtoken(end, &end);
        }
    }

    add_Target(data, target);

    free(target.label);

    return S_OK;
}

void LogResult(const Base *base, const Target *tgt, const Ammo_t *ammo, const BallisticResult *res)
{
    if (!base || !tgt || !ammo || !res)
    {
        printf("LogResult: invalid argument.\n");
        return;
    }

    char folder_buffer[1024];
    strcpy(folder_buffer, folder_path);
    strcat(folder_buffer, "log.txt");
    FILE *fp = fopen(folder_buffer, "a");
    if (!fp)
    {
        printf("Failed to open log.txt for writing.\n");
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

int HandleParameterSelection(void)
{
    unsigned baseID, targetID, ammoID;
    char input[128] = {0};
    printf("------------------------------\n");
    printf("Enter parameters: <base_id> <target_id> <ammo_id>\n");
    printf("Type [back] to return or to view/expand the available [list]!\n");
    printf("------------------------------\n\n");

    if (!fgets(input, sizeof(input), stdin))
        return -1;

    input[strcspn(input, "\r\n")] = 0; // remove newline

    if (isalpha((unsigned char)input[0]))
    {
        Command_t cmd = ParseCommandFromBuffer(input, CMD_C_BACK, NULL);
        if (cmd == CMD_BACK)
            return -1;
        if (cmd != CMD_INVALID)
            return 0;
    }

    if (sscanf(input, "%u %u %u", &baseID, &targetID, &ammoID) != 3)
    {
        printf("Invalid input. Expected format: <base_id> <target_id> <ammo_id>\n");
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
    if (!base || !ammo || !compute_fire_solution(base, tgt, ammo).valid)
    {
        printf("Finding best available base and ammo...\n");
        printf("This may take a while...\n");
        if (!SuggestBestBaseAndAmmo_RegionAware(base_list, tgt, &base, &ammo))
        {
            printf("No suitable base/ammo combination found.\n");
            return 0;
        }

        printf("Suggested base and ammo:\n");
        printf("  Base ID: %u  Label: %s  Coordinates: lat %.6f, lon %.6f, alt %.2f m\n",
               base->id, base->label, base->coord.latitude, base->coord.longitude, base->coord.altitude);
        printf("  Ammo ID: %u  Label: %s\n", ammo->id, ammo->name);

        printf("Would you like to accept these changes? [yes/no]\n");
        if (GetCommand(CMD_C_LOOP | CMD_C_YESNO, NULL) == CMD_YES)
        {
            goto Recalculate;
        }

        return 0;
    }

Recalculate:
    BallisticResult b_result = compute_fire_solution(base, tgt, ammo);
    if (!b_result.valid)
    {
        printf("Even the suggested base cannot reach the target.\n");
        return 0;
    }

    LogResult(base, tgt, ammo, &b_result);
    printf("Parameters accepted and logged.\n");
    return 0;
}
