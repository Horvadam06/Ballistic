#include "command\commands.h"
#include "command\list_wizard\wizard.h"
#include "helpers\color.h"
#include "helpers\helpers.h"
#include "memory\load.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int general_state_options(const char *const array[], int n, int protected_pos, int step)
{
    if (protected_pos < -1)
        protected_pos = -1;
    int start_i = protected_pos + 1;
    int adjusted_step = start_i + step - 1;
    if (adjusted_step >= n)
    {
        Error_message("For the array: 0x%p, the given step: %d, exceeds the array's limit: %d!\n", array, adjusted_step, n);
        return 0;
    }
    else if (adjusted_step <= protected_pos)
    {
        Error_message("For the array: 0x%p, the value: %d, is protected!\n", array, step);
        return 0;
    }

    if (protected_pos >= 0)
        add_header(NORMAL_SEPERATOR_SIZE, ANSI_GRAY, "%s", array[0]);

    for (int i = start_i; i < n; i++)
        printf(COLOR_RESET " %d.%s %s type shiii\n",
               i - start_i + 1, (i == adjusted_step) ? "" : COLOR_HINT, array[i]);

    add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);

    if (protected_pos >= 1)
        printf(COLOR_HINT "%s (%d/%d) : " COLOR_RESET "%s\n",
               array[1], step, n - protected_pos - 1, array[adjusted_step]);
    // add_header(NORMAL_SEPERATOR_SIZE, ANSI_GRAY, COLOR_HINT "%s (%d/%d) | " COLOR_RESET "%s",
    //             array[1], step, n - protected_pos - 1, array[adjusted_step]);

    return 1;
}



P_HRESULT base_string_state_token(char *buffer, size_t buffer_n, char *token, size_t token_n, int state)
{
    (void)buffer_n;
    (void)token_n;
    // plan
    // search for the required token with strchr, if it's in the 0th pos, it's a token for sure, if not
    // check if there is an empty space before it, if there is it's a token for sure
    // THIS CHECK CAN ONLY WORK IF I DON'T ALLOW SPACES IN FOR EXAMPLE LSOMETHING CLA, can only work with SOMETHING_CLA,
    // maybe rework the phraser, to convert the user input to automaticly add _ in the place of spaces

    // PSUEDO PLAN (or something)
    //   check the state validity
    //   check the input type validity
    //   search for the token id in the buffer
    //       if (exists)
    //           replace it, and offset everything by the pre calculated input length
    //       else
    //           add the id to it & slap it to the end, and let the handler take care of it
    //
    //

    const int STATE_CONFIRM = 6;
    // state validity check5
    if (state > STATE_CONFIRM || state < 1)
        return P_S_FALSE;
    else if (state == STATE_CONFIRM)
    {
        return handle_base(buffer, base_list, NULL);
    }

    state--;
    const char *id[] = {
        "I",          // id
        "L",          // label
        "CLA", "CLO", // coordinates (latitude, longitude)[degree]
        "CA",         // altitude [meter]
        "AMS"         // ammo stock
    };

    // merge the 2 together somehow, it works via the state, and ids array

    // check if it exists already in the buffer

    // id
    // label
    // coordinate
    // height
    // ammo stock

    switch (state)
    {
    case 0:
    {
        if (replace_token_a(buffer, id[0], token, validate_int) == P_S_OK)
            return P_S_PARTIAL;
        break;
    }
    case 1:
    {
        if (replace_token_a(buffer, id[1], token, NULL) == P_S_OK)
            return P_S_PARTIAL;
        break;
    }
    case 2:
    {
        // replace_token_a(buffer, id[2], token, NULL);
        // replace_token_a(buffer, id[3], token, NULL);
        break;
    }
    case 3:
    {
        if (replace_token_a(buffer, id[4], token, validate_int) == P_S_OK)
            return P_S_PARTIAL;
        break;
    }
    case 4:
    {

        break;
    }

    default:
        return P_S_FALSE;
    }

    return P_S_OK;
}

P_HRESULT target_string_state_token(char *buffer, size_t buffer_n, char *token, size_t token_n, int state)
{
    (void)buffer_n;
    (void)state;
    (void)token;
    (void)token_n;
    (buffer)[0] = 's';
    return P_S_FALSE;
}

P_HRESULT ammo_t_string_state_token(char *buffer, size_t buffer_n, char *token, size_t token_n, int state)
{
    (void)buffer_n;
    (void)state;
    (void)token;
    (void)token_n;

    (buffer)[0] = 's';
    return P_S_FALSE;
}

P_HRESULT base_info_print(ListManagerConfig *config, DisplayType display, void *entry_ptr, void *data)
{
    (void)config;
    (void)data;
    switch (display)
    {
    case LIST_MENU_ENTRY:
        printf("\n");
        add_header(NORMAL_SEPERATOR_SIZE, ANSI_GRAY, "%-*s " ANSI_GRAY "|" COLOR_RESET "%s", HELP_DESCRIPTION_ALIGMENT, "Bases menu", "command");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "list" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Display list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "add" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Add to list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "delete" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Delete from list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "back" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Step back");
        add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(ANSI_RESET);
        break;

    case LIST_MENU_ADD:

        // phrase state data pointer to state
        Debug_message("Print base add\n");
        break;

    case LIST_MENU_DELETE:
        printf("\n Delete base\n");
        add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(
            COLOR_INFO
            " Enter the ID of the base to delete.\n"
            " Type 'back' to return to the previous menu.\n");
        add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        break;

    case LIST_MENU_DEL_CONFIR:
    {
        Base *b = (Base *)entry_ptr;
        printf("\n Selected base:\n");
        printf("   ID: %u\n", b->id);
        printf("   Label: %s\n", b->label);
        printf("   Location: %.3f°, %.3f°\n", b->coord.latitude, b->coord.longitude);
        printf("   Altitude: %.2f m\n", b->coord.altitude);
        printf("\n");
        break;
    }
    default:
        Error_message("Missing display type!\n");
    }

    return P_S_OK;
}

P_HRESULT target_info_print(ListManagerConfig *config, DisplayType display, void *entry_ptr, void *data)
{
    (void)config;
    (void)data;
    switch (display)
    {
    case LIST_MENU_ENTRY:
        printf("\n");
        add_header(NORMAL_SEPERATOR_SIZE, ANSI_GRAY, "%-*s " ANSI_GRAY "|" COLOR_RESET "%s", HELP_DESCRIPTION_ALIGMENT, "Targets menu", "command");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "list" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Display list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "add" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Add to list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "delete" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Delete from list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "back" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Step back");
        add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(ANSI_RESET);
        break;

    case LIST_MENU_ADD:
        Debug_message("print target add\n");
        break;

    case LIST_MENU_DELETE:
        printf("\n Delete target\n");
        add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(
            COLOR_INFO
            " Enter the ID of the target to delete.\n"
            " Type 'back' to return to the previous menu.\n");
        add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        break;

    case LIST_MENU_DEL_CONFIR:
    {
        Target *t = (Target *)entry_ptr;
        printf("\n Selected target:\n");
        printf("   ID: %u\n", t->id);
        printf("   Label: %s\n", t->label);
        printf("   Location: %.3f°, %.3f°\n", t->coord.latitude, t->coord.longitude);
        printf("   Altitude: %.2f m\n", t->coord.altitude);
        printf("   Area: %.2f m²\n", t->area);
        printf("\n");
        break;
    }
    default:
        Error_message("Missing display type!\n");
    }
    return P_S_OK;
}

P_HRESULT ammo_info_print(ListManagerConfig *config, DisplayType display, void *entry_ptr, void *data)
{
    (void)config;
    (void)data;
    switch (display)
    {
    case LIST_MENU_ENTRY:
        printf("\n");
        add_header(NORMAL_SEPERATOR_SIZE, ANSI_GRAY, "%-*s " ANSI_GRAY "|" COLOR_RESET "%s", HELP_DESCRIPTION_ALIGMENT, "Ammo type menu", "command");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "list" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Display list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "add" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Add to list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "delete" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Delete from list");
        printf(ANSI_GRAY " %-*s " VLINE COLOR_TITLE "back" ANSI_GRAY "\n", HELP_DESCRIPTION_ALIGMENT, "Step back");
        add_seperator_s(ANSI_GRAY, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(ANSI_RESET);

        break;
    case LIST_MENU_ADD:

        Debug_message("print ammo_t add\n");

        break;
    case LIST_MENU_DELETE:
        printf("\n Delete ammo\n");
        add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        printf(
            COLOR_INFO
            " Enter the ID of the ammo to delete.\n"
            " Type 'back' to return to the previous menu.\n");
        add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
        break;
    case LIST_MENU_DEL_CONFIR:
        if (!entry_ptr)
        {
            Error_message("Missing node ptr!\n");
            return P_S_FALSE;
        }
        Ammo_t *a = (Ammo_t *)entry_ptr;
        printf("\n Selected ammo:\n");
        printf("   ID: %u\n", a->id);
        printf("   Label: %s\n", a->name);
        printf("   Mass: %f\n", a->weight);
        printf("   Bore: %f\n", a->diameter);
        printf("\n");
        break;
    default:
        Error_message("Missing display type!\n");
    }

    return P_S_OK;
}
