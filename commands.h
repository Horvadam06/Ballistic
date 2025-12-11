/**
 * @defgroup CommandHandling Command Processing
 * @brief Interactive command-line interface and user interaction
 * 
 * This module handles all user commands, provides help system,
 * and manages the interactive command loop with modal support.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include "error.h"
#include <stdint.h>

/**
 * @brief Command type identifiers
 * @ingroup CommandHandling
 */
typedef enum
{
    CMD_INVALID,        ///< Invalid or unknown command
    CMD_EXIT,           ///< Exit program
    CMD_HELP,           ///< Show help
    CMD_CONTINUE,       ///< Continue operation
    CMD_YES,            ///< Yes response
    CMD_NO,             ///< No response
    CMD_SAVE,           ///< Save data
    CMD_CANCEL,         ///< Cancel operation
    CMD_SELECT,         ///< Select option
    CMD_BACK,           ///< Go back/return
    CMD_CALCULATE,      ///< Calculate trajectory
    CMD_RESET,          ///< Reset system
    CMD_TARGET,         ///< Target operations
    CMD_BASE,           ///< Base operations
    CMD_AMMO,           ///< Ammunition operations
    CMD_TEST,           ///< Run test
    CMD_LIST,           ///< List items
    CMD_ADD,            ///< Add item
    CMD_DELETE,         ///< Delete item
    CMD_COLOUR,         ///< Set color mode
    CMD_MONO,           ///< Set monochrome mode
    CMD_CLEAR_TERMINAL  ///< Clear terminal
} Command_t;

/**
 * @brief Command context flags for modal operation
 * 
 * Bitmask flags defining which commands are available in each context.
 * @ingroup CommandHandling
 */
typedef enum
{
    CMD_C_DEFAULT = 1 << 0,  ///< Default commands with handlers
    CMD_C_GENERIC = 1 << 1,  ///< Generic commands without handlers
    CMD_C_SYSTEM = 1 << 2,   ///< System commands (exit, help, etc.)
    CMD_C_YESNO = 1 << 3,    ///< Yes/No commands
    CMD_C_LOAD = 1 << 4,     ///< Data loading commands
    CMD_C_SETUP = 1 << 5,    ///< Setup/configuration commands
    CMD_C_EXIT = 1 << 6,     ///< Exit-related commands
    CMD_C_CALC = 1 << 7,     ///< Calculation commands
    CMD_C_SELECT = 1 << 8,   ///< Selection commands
    CMD_C_BACK = 1 << 9,     ///< Back/return commands
    CMD_C_CANCEL = 1 << 10,  ///< Cancel commands
    CMD_C_NOHANDLE = 1 << 11,///< Cancel's out the commands handler
    CMD_C_NOCOMMENT = 1 << 12,///< Cancel's out the commands stdout
    CMD_C_LOOP = 1 << 20     ///< Special: loop until valid input
} Command_case;

/**
 * @brief All command contexts combined
 * @ingroup CommandHandling
 */
#define CMD_C_ALL 0xFFFFFFFFu

/**
 * @brief Command definition structure
 * @ingroup CommandHandling
 */
typedef struct
{
    const char *name;        ///< Command name (lowercase)
    Command_t type;          ///< Command type identifier
    void (*handler)(const char *args, Command_case case_flags, PROCESS *running); ///< Command handler function
    const char *description; ///< Help description
    uint32_t case_mask;      ///< Bitmask of contexts where command is available
} Command;

/**
 * @name Core Command Functions
 * @{
 */

/**
 * @brief Get command from user with modal context
 * 
 * Reads user input, validates against available commands in current context,
 * and executes the appropriate handler.
 * 
 * @param case_flags Bitmask defining available commands in current context
 * @param running Pointer to program state (may be modified by handlers)
 * @return Command_t identifier of executed command
 * 
 * @note If CMD_C_LOOP is set, function loops until valid command received
 * @note Normalizes input to lowercase before comparison
 * 
 * @ingroup CommandHandling
 */
Command_t GetCommand(Command_case case_flags, PROCESS *running);

/**
 * @brief Parse command from buffer (non-interactive)
 * 
 * Similar to GetCommand but works with pre-existing input buffer.
 * Useful for parsing commands from files or other sources.
 * 
 * @param input Input buffer containing command
 * @param case_flags Bitmask of allowed commands
 * @param running Pointer to program state
 * @return Command_t identifier
 * @ingroup CommandHandling
 */
Command_t ParseCommandFromBuffer(char *input, Command_case case_flags, PROCESS *running);

/**
 * @brief Normalize input string to lowercase
 * @param input String to normalize (modified in-place)
 * @ingroup CommandHandling
 */
void NormalizeInput(char *input);



/** @} */

/**
 * @name Command Handler Functions
 * 
 * Each handler corresponds to a specific command. Handlers receive
 * arguments string and current context flags.
 * @{
 */

/**
 * @brief Exit program handler
 * @param args Command arguments (unused)
 * @param case_flags Current context flags
 * @param running Pointer to program state (set to S_END)
 * @ingroup CommandHandling
 */
void cmd_exit(const char *args, Command_case case_flags, PROCESS *running);

/**
 * @brief Help command handler
 * @param args Command arguments (unused)
 * @param case_flags Current context flags (determines which commands to show)
 * @param running Pointer to program state (unused)
 * @ingroup CommandHandling
 */
void cmd_help(const char *args, Command_case case_flags, PROCESS *running);

/**
 * @brief Base operations handler
 * @param args Command arguments
 * @param case_flags Current context flags
 * @param running Pointer to program state
 * @ingroup CommandHandling
 */
void cmd_base(const char *args, Command_case case_flags, PROCESS *running);

/**
 * @brief Calculate trajectory handler
 * @param args Command arguments
 * @param case_flags Current context flags
 * @param running Pointer to program state
 * @ingroup CommandHandling
 */
void cmd_calculate(const char *args, Command_case case_flags, PROCESS *running);

/**
 * @brief Ammunition operations handler
 * @param args Command arguments
 * @param case_flags Current context flags
 * @param running Pointer to program state
 * @ingroup CommandHandling
 */
void cmd_ammo(const char *args, Command_case case_flags, PROCESS *running);

/**
 * @brief Target operations handler
 * @param args Command arguments
 * @param case_flags Current context flags
 * @param running Pointer to program state
 * @ingroup CommandHandling
 */
void cmd_target(const char *args, Command_case case_flags, PROCESS *running);

/**
 * @brief Test system handler
 * @param args Command arguments
 * @param case_flags Current context flags
 * @param running Pointer to program state
 * @ingroup CommandHandling
 */
void cmd_test(const char *args, Command_case case_flags, PROCESS *running);

/**
 * @brief Configuration handler
 * @param args Command arguments
 * @param case_flags Current context flags
 * @param running Pointer to program state
 * @ingroup CommandHandling
 */
void cmd_config(const char *args, Command_case case_flags, PROCESS *running);

/**
 * @brief Set color mode handler
 * @param args Command arguments
 * @param case_flags Current context flags
 * @param running Pointer to program state
 * @ingroup CommandHandling
 */
void cmd_colour(const char *args, Command_case case_flags, PROCESS *running);

/**
 * @brief Set monochrome mode handler
 * @param args Command arguments
 * @param case_flags Current context flags
 * @param running Pointer to program state
 * @ingroup CommandHandling
 */
void cmd_mono(const char *args, Command_case case_flags, PROCESS *running);

/**
 * @brief Clear terminal handler
 * @param args Command arguments
 * @param case_flags Current context flags
 * @param running Pointer to program state
 * @ingroup CommandHandling
 */
void cmd_clr_ter(const char *args, Command_case case_flags, PROCESS *running);

/**
 * @brief Save lists handler
 * @param args Command arguments
 * @param case_flags Current context flags
 * @param running Pointer to program state
 * @ingroup CommandHandling
 */
void cmd_save(const char *args, Command_case case_flags, PROCESS *running);

/** @} */

/**
 * @brief Global command table
 * 
 * Array of all available commands with their handlers and contexts.
 * @ingroup CommandHandling
 */
extern Command commands[];

/**
 * @brief Number of commands in command table
 * @ingroup CommandHandling
 */
extern int command_count;

#endif /* COMMANDS_H */