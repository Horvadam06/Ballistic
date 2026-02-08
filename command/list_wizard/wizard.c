#include "wizard.h"
#include "helpers\color.h"
#include "helpers\helpers.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void base_add_wizard(ListManagerConfig *cfg)
{
    (void)cfg;
    const char *fn_name = "base_add_wizard";
    Debug_message("Entering '%s'...\n", fn_name);
    
    /*Base_Draft draft;
    base_draft_init(&draft);

    int running = 1;

    while (running)
    {
        clear_terminal();

        base_wizard_render(&draft);

        char input[256];
        fgets(input, sizeof(input), stdin);
        strip_newline(input);

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

                char buffer[1024];
                base_draft_serialize(&draft, buffer, sizeof(buffer));

                handle_base(buffer, cfg->list_ptr, NULL);
                running = 0;
                break;
            }

            case WIZARD_CANCEL:
                running = 0;
                break;
        }
    }

    base_draft_free(&draft);*/

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
    buffer[strcspn(buffer, "\r\n")] = 0;
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
            buffer[strcspn(buffer, "\r\n")] = 0;
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
