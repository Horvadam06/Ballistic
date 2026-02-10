#include "wizard.h"
#include "helpers\color.h"
#include "helpers\helpers.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static void base_draft_init(Base_Draft *d)
{
    memset(d, 0, sizeof(*d));
    d->state = 1;
    d->ammo_stock = NULL;
}

static void base_wizard_render(Base_Draft *d)
{
    const char *steps[] = {
        "Enter ID",
        "Enter label",
        "Enter latitude, longitude",
        "Enter altitude",
        "Enter ammo stock (id:N,...)",
        "Confirm"};

    const int total = SIZE_OF_STRING_ARRAY(steps);
    const int current = d->state;

    add_header(NORMAL_SEPERATOR_SIZE, ANSI_GRAY, "%s", "Add base wizard");

    for (int i = 0; i < total; ++i)
    {
        const int step = i + 1;
        const bool active = (step == current);

        printf(COLOR_RESET " %d.%s %-32s ",
               step,
               active ? "" : COLOR_HINT,
               steps[i]);

        switch (i + 1)
        {
        case 1:
            if (d->has_id)
                printf("[%u]", d->id);
            else
                printf("[nan]");
            break;

        case 2:
            if (d->has_label)
                printf("[%s]", d->label);
            else
                printf("[nan]");
            break;

        case 3:
            if (d->has_coord)
                printf("[%.6f, %.6f]",
                       d->coordinate.latitude,
                       d->coordinate.longitude);
            else
                printf("[nan]");
            break;

        case 4:
            if (d->has_altitude)
                printf("[%.2f]", d->coordinate.altitude);
            else
                printf("[nan]");
            break;

        case 5:
            printf("[%s]", d->has_ammo_stock ? "ok" : "nan");
            break;

        default:
            break;
        }

        printf("\n");
    }

    add_seperator_s(COLOR_HINT, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);

    printf(COLOR_HINT "Add base (%d/%d): " COLOR_RESET "%s\n",
           current,
           total,
           steps[current - 1]);

    printf("\nType sN to switch state (e.g. s2)\n");
    printf("Or enter value for current state.\n");
    printf("> ");
}

static void base_draft_free(Base_Draft *d)
{
    free(d->label);
    d->label = NULL;

    if (d->ammo_stock)
    {
        free_Ammo_Stock_List(&d->ammo_stock);
        d->ammo_stock = NULL;
    }
}

static int base_draft_is_complete(Base_Draft *d)
{
    int succeded = 1;

    if (!d->has_id)
    {
        Error_message("Missing id!\n");
        succeded = 0;
    }
    if (!d->has_label)
    {
        Error_message("Missing label!\n");
        succeded = 0;
    }
    if (!d->has_coord)
    {
        Error_message("Missing coordinate(s)!\n");
        succeded = 0;
    }
    if (!d->has_altitude)
    {
        Error_message("Missing altitude!\n");
        succeded = 0;
    }
    if (!d->has_ammo_stock)
    {
        Error_message("Missing ammo stock!\n");
        succeded = 0;
    }

    return succeded;
}

static WizardAction base_wizard_handle_input(Base_Draft *d, char *input)
{
    int new_state;

    if (parse_state_selector(input, &new_state))
    {
        d->state = clamp_with_warning(new_state, 1, 6);

        return WIZARD_CONTINUE;
    }
    char *end = NULL;

    switch (d->state)
    {
    case 1: // ID
    {
        unsigned id = (unsigned)strtoul(input, &end, 10);

        if (errno == ERANGE || *end != '\0')
        {
            Error_message("Out of range '%u'\n", id);
            errno = 0;
            return WIZARD_CONTINUE;
        }

        d->id = id;
        d->has_id = true;
        d->state++;
        break;
    }

    case 2: // Label
    {
        free(d->label);

        d->label = malloc(strlen(input) + 1);
        strcpy(d->label, input);

        d->has_label = true;
        d->state++;
        break;
    }

    case 3: // Latitude, Longitude
    {
        Coordinate tmp_crd;
        tmp_crd.altitude = d->coordinate.altitude;
        input = skip_spaces(input);

        for (int i = 1; i <= 2; i++)
        {
            double v = strtod(input, &end);
            input = strtoken(end, &end);
            if (i == 1)
                tmp_crd.latitude = v;
            else
                tmp_crd.longitude = v;

            if (errno == ERANGE || !validate_coordinate(tmp_crd))
            {
                Error_message("Out of range '%f'\n", v);
                errno = 0;
                return WIZARD_CONTINUE;
            }
        }

        d->coordinate = tmp_crd;
        d->has_coord = true;

        d->state++;
        break;
    }

    case 4: // Altitude
    {
        double v = strtod(input, &end);

        if (errno == ERANGE || *end != '\0')
        {
            Error_message("Out of range '%f'\n", v);
            errno = 0;
            return WIZARD_CONTINUE;
        }

        d->coordinate.altitude = v;
        d->has_altitude = true;
        d->state++;
        break;
    }

    case 5: // Ammo
    {
        Ammo_Stock_List *tmp_list = NULL;
        init_Ammo_Stock_List(&tmp_list);
        unsigned type_cnt = list_count_nodes(ammo_t_list->head, ammo_t_list->tail, offsetof(Ammo_t_Node, next), ammo_t_node_valid, NULL);

        while (strlen(input) >= 1)
        {
            unsigned id, amount = 0;

            for (int i = 1; i <= 2; i++)
            {
                unsigned v = (unsigned)strtoul(input, &end, 10);
                input = strtoken(end, &end);

                if (i == 1)
                    id = v;
                else
                    amount = v;

                if (errno == ERANGE || (i == 1) ? v > type_cnt : 0)
                {
                    Error_message("Invalid %s '" ANSI_BRIGHT_RED "%u" COLOR_ERROR "'!\n", (i == 1) ? "id" : "amount", v);
                    errno = 0;
                    if (i == 1)
                    {
                        input = strtoken(end, &end);
                        i = 0;
                        continue;
                    }
                }
            }

            // input = skip_spaces(input);
            add_Ammo_Stock(tmp_list, GetNode_Ammo_t_List(ammo_t_list, id), amount);
        }

        if (d->ammo_stock)
        {
            free_Ammo_Stock_List(&d->ammo_stock);
        }

        d->ammo_stock = tmp_list;

        d->has_ammo_stock = (list_count_nodes(d->ammo_stock->head, d->ammo_stock->tail, offsetof(Ammo_Stock, next), ammo_t_node_valid, NULL) != 0);
        d->state++;
        break;
    }

    case 6: // Confirm
    {
        if (!base_draft_is_complete(d))
        {
            printf("Missing required fields!\n");
            d->state = 1;
            return WIZARD_CONTINUE;
        }

        printf("Save? ");
        cmd_help_inline(NULL, CMD_C_YESNO, NULL);
        putchar('\n');

        if (GetCommand(CMD_C_MODAL | CMD_C_YESNO, NULL) == CMD_YES)
            return WIZARD_CONFIRM;

        d->state = 1;
        return WIZARD_CONTINUE;
    }

    default:
        break;
    }

    return WIZARD_CONTINUE;
}

void base_add_wizard(ListManagerConfig *cfg)
{
    (void)cfg;
    const char *fn_name = "base_add_wizard";
    Debug_message("Entering '%s'...\n", fn_name);

    Base_Draft draft;
    base_draft_init(&draft);

    int running = 1;

    while (running)
    {
        clear_terminal();

        base_wizard_render(&draft);

        char input[1024];
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\r\n")] = '\0';

        WizardAction action = base_wizard_handle_input(&draft, input);

        switch (action)
        {
        case WIZARD_CONTINUE:
            break;

        case WIZARD_CONFIRM:
        {
            if (!base_draft_is_complete(&draft))
            {
                Error_message("Missing required fields!\n");
                break;
            }

            // char buffer[1024];
            // base_draft_serialize(&draft, buffer, sizeof(buffer));

            // handle_base(buffer, cfg->list_ptr, NULL);
            running = 0;
            break;
        }

        case WIZARD_CANCEL:
            running = 0;
            break;
        }
    }

    base_draft_free(&draft);

    Debug_message("Exiting '%s'...\n", fn_name);
}

/**
 * @brief modal, general_list_menu helper
 * @return 0 Entry_no_clr |
 * @return 1 Entry
 */
static int general_list_menu_add(ListManagerConfig *config)
{
    // it's preaty whacky, should revise it later
    int state = 1;
    char handler_buffer[1024] = {'\0'};
Entry_add:
    clear_terminal();
Entry_add_after_clr:
    printf(CURSOR_HIDE);
    if (config->print_(config, LIST_MENU_ADD, handler_buffer, &state) == P_S_FALSE)
    {
        printf(CURSOR_SHOW);
        return 0;
    }

Entry_add_no_clr:

    char buffer[1024];
    // write the input into buffer, and process if back request
    printf("> " CURSOR_SHOW);
    Command_t add_cmd = CMD_INVALID;

    /*if (edg_trig_inp == '\t')
        add_cmd = CMD_NEXT;

    else
    {*/

    // ungetc(edg_trig_inp, stdin);
    // putchar(edg_trig_inp);
    fgets(buffer, sizeof(buffer), stdin);
    // remove traling carrig feed or new line
    buffer[strcspn(buffer, "\r\n")] = '\0';
    add_cmd = ParseCommandFromBuffer(buffer, CMD_C_BACK | CMD_C_SWITCH | CMD_C_NOCOMMENT, NULL);

    switch (add_cmd)
    {
    case CMD_MODAL:
        goto Entry_add_no_clr;
        break;
    case CMD_BACK:
        return 1;
        break;
    case CMD_NEXT:
        Debug_message("Will increment\n");
        goto Entry_add;
        break;
    case CMD_PREV:
        Debug_message("Will decrement\n");
        goto Entry_add;
        break;

    default:
        if (parse_state_selector(buffer, &state))
        {
            clear_terminal();
            Debug_message("State changed to: %d\n", state);
            goto Entry_add_after_clr;
        }
        break;
    }

    /*
    int req = back_request(buffer, sizeof(buffer));
    if (req == 1)
        return 1;
    if (req == -1)
        goto Entry_add_no_clr;

    // remove traling carrig feed or new line
    buffer[strcspn(buffer, "\r\n")] = 0;

    // remove when proper command handling
    if (!isalpha((unsigned char)buffer[0]))
    {
        state = (int)strtof(buffer, NULL);
        goto Entry_add;
    }*/

    // handle input error
    // option 1.
    // make it so it's somewhat self contained, it return if the token was correct with S_OK maybe, if no S_FALSE
    // probably a stand alone enum state machine would be the best, which returns the process state (finished, running, etc)
    // Always return's to Entry_add, so the command handling is already processed. If it signals finished,
    // only then continue with the process
    // ammo and target should be the easiest, bases ammo stock is the problem
    // Biggest issues:a very big chance of memory leak
    //
    // option 2.
    // make it fully self contained
    // pros: memory safe
    // cons : duplicated code for command handling, maybe can be avoided if the command managment is modified to be self contained

    // self contained command handling parameters, back, help, and that's it. Maybe can be avoided using buffer command handler???

    // something is needed, which ectracts the tyoe and the data from the token, and checks it's validity,
    //  if it's valid, then it writes it to the temp data type, on exit, or succes, it free's up all of the allocated memory, then calls the appripiate call function
    // basicly a per token handler
    //
    // the cleanest would be a self contained function, because then it won't pollute the command logic further,
    // with backend stuff, so should be at load.c or storage.c, the only problem would be, that it would cause the "gui" to freeze, or maybe just move the ui displaying to there
    // basicly it should be the same as the deticated handler, the only difference would be, that instead of the tokenizer stepping on the buffer, it would only phrase
    // one token
    //
    // this is a self contained loop, maybe create an internal buffer, which get's modified in the
    //"handler", it would string together the input data, and when finished pass it to the "real" handler??
    // the data can be raw, so no id tokens are needed, because they type would depend on the state
    //

    // maybe pass the struct to the function too, so it can display the value set, and not just rawdogging the value's by head
    // handler should be seperate, later I need to add it back
    // config->handler(buffer, config->list_ptr, NULL) == P_S_FALSE
    // internal state handling or some fuck
    if (!config->phrase_tokens)
    {
        Error_message("Missing token parser!\n");
        return 0;
    }
    P_HRESULT hr = config->phrase_tokens(
        handler_buffer, SIZE_OF_CHAR_ARRAY(handler_buffer),
        buffer, SIZE_OF_CHAR_ARRAY(buffer),
        state);
    switch (hr)
    {
    case P_S_OK:
        break;
    case P_S_PARTIAL:
        printf(COLOR_SUCCESS "Value updated!\n" COLOR_RESET);
        goto Entry_add_no_clr;
        break;
    case P_S_FALSE:
        clear_terminal();
        Error_message("Invalid input!\n");
        goto Entry_add_after_clr;
        break;

    default:
        break;
    }

    printf(COLOR_SUCCESS "idgaf about wtf is goin' on\n" COLOR_RESET);
    goto Entry_add_no_clr;

    return 0;
}

void general_list_menu(Command_case case_flags, ListManagerConfig *config)
{
    // function parameters
    //   -case_flags
    //
    //   -display function display data by call case display_base(Command_t cmd_t) this way it's static only,
    //    could make a unique enum for the display options
    //
    //   -list_print fp
    //
    //   -add_handler fp
    //
    //   -search_id fp
    //
    //   -delete_handler fp

    // 1. entry text
    // 3. add parameters
    // 4. add handler
    // 5. delete parameters

    // 2. list_print pointer
    // 6. find by id
    // 7. delete handler

Entry:
    clear_terminal();
Entry_no_clr:
    if (!(case_flags & CMD_C_NOCOMMENT))
    {
        if (!config->print_)
        {
            Error_message("Missing print_ parameter for %s!\n", config->name);
            return;
        }
        config->print_(config, LIST_MENU_ENTRY, NULL, NULL);
    }
    Command_t cmd;

    while ((cmd = GetCommand(CMD_C_LOAD | CMD_C_MODAL, NULL)) != CMD_BACK)
    {
        switch (cmd)
        {
        case CMD_LIST:
            config->print_list(config->list_ptr);
            return;
        case CMD_ADD:
        {
            Debug_message("case 'CMD_ADD'\n");
            if (config->on_add)
            {
                Debug_message("case 'on_add'\n");
                config->on_add();
            }
            else
            {
                Debug_message("Missing 'on_add'!\n");
                if (general_list_menu_add(config))
                    goto Entry;
                else
                    goto Entry_no_clr;
            }
            break;
        }
        case CMD_DELETE:
        {
            clear_terminal();
            config->print_(config, LIST_MENU_DELETE, NULL, NULL);

        Entry_delete_no_clr:

            char buffer[1024];
            // write the input into buffer, and process if back request
            int req_val = back_request(buffer, sizeof(buffer));
            if (req_val == 1)
                goto Entry;
            else if (req_val == -1)
                goto Entry_delete_no_clr;

            // remove trailing carrige return or new line
            buffer[strcspn(buffer, "\r\n")] = '\0';
            // extract id
            unsigned id = (unsigned)strtoul(buffer, NULL, 10);

            // handle id
            void *node = NULL;
            if (isalpha((unsigned char)buffer[0]) || !(node = config->find_by_id(config->list_ptr, id)))
            {
                clear_terminal();
                Error_message("Invalid input!\n");
                goto Entry_no_clr;
            }
            else
            {
                config->print_(config, LIST_MENU_DEL_CONFIR, node, NULL);

                // confirm deletion
                printf("\n Confirm deletion? ");
                cmd_help_inline(NULL, CMD_C_YESNO, NULL);

                printf("\n");
                if (GetCommand(CMD_C_MODAL | CMD_C_YESNO, NULL) == CMD_NO)
                {
                    printf(COLOR_ERROR " Operation cancelled.\n" COLOR_RESET);
                    goto Entry_delete_no_clr;
                }

                // deletion error handling
                if (config->delete_by_id(config->list_ptr, id) != -1)
                    printf(COLOR_SUCCESS " List updated!\n");
                else
                    Error_message(" Delete failed!\n");
                printf(ANSI_RESET);
                goto Entry_delete_no_clr;
            }
            break;
        }
        default:
            break;
        }
        Debug_message("Waiting for list menu command...\n");
    }
    cmd_clr_ter(NULL, CMD_C_ALL, NULL);
}

//==================================================
// Public wizard helpers
//==================================================

int parse_state_selector(const char *s, int *out_state)
{

    if (tolower(s[0]) != 's')
        return 0;
    if (!isdigit((unsigned char)s[1]))
        return 0;

    char *end;
    long v = strtol(s + 1, &end, 10);

    if (*end != '\0')
        return 0;

    *out_state = (int)v;
    return 1;
}
