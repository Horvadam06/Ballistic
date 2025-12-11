/**
 * @defgroup Loading File Loading and Configuration
 * @brief File parsing, configuration management, and data loading
 * 
 * This module handles all file I/O operations including configuration,
 * data file parsing with custom format, and logging of results.
 */

#ifndef LOAD_H
#define LOAD_H

#include "storage.h"
#include "error.h"
#include "calculations.h"
#include <stdio.h>

/**
 * @brief Parser state machine states
 * @ingroup Loading
 */
typedef enum
{
    NORMAL,        ///< Reading normal data
    LINE_COMMENT,  ///< Inside // comment
    BLOCK_COMMENT, ///< Inside /* ... */ comment
    RECORD         ///< Collecting record text until ';'
} ParseState;

/**
 * @brief Input parsing flags
 * @ingroup Loading
 */
typedef enum
{
    T_ID,        ///< Identifier token
    T_LABEL,     ///< Label/name token
    T_DATA,      ///< Data value token
    T_WHITESPACE ///< Whitespace token
} InputFlags;

/**
 * @brief Default data field flags for parsing
 * @ingroup Loading
 */
typedef enum
{
    F_ID = 0,        ///< ID field flag
    F_LABEL = 1,     ///< Label field flag
    F_LONGITUDE = 2, ///< Longitude field flag
    F_LATITUDE = 3,  ///< Latitude field flag
    F_ALTITUDE = 4   ///< Altitude field flag
} Default_flags;

/**
 * @brief Pointer to current working directory path
 * 
 * Set via configuration file. All data files are loaded relative to this path.
 * @ingroup Loading
 */
extern char *folder_path;

/**
 * @brief Set the working directory path
 * @param new_path New directory path
 * @ingroup Loading
 */
void set_folder_path(const char *new_path);

/**
 * @brief Free the working directory path memory
 * @ingroup Loading
 */
void free_folder_path(void);

/**
 * @brief Record handler function type for parser
 * 
 * Callback function called by parser for each complete record.
 * @param record Complete record string (without trailing ';')
 * @param data User data passed to parser
 * @param error_flags Array to indicate missing required fields
 * @return HRESULT status code
 * @ingroup Loading
 */
typedef HRESULT (*RecordHandler)(char *record, void *data, int *error_flags);

/**
 * @brief Parse a file and call handler for each record
 * 
 * Handles comments (// and \* *\) and record separation by ';'.
 * @param filename File to parse
 * @param handler Callback function for each record
 * @param data User data passed to handler
 * @param mode Process mode (S_DEBUG enables debugging output)
 * @return S_OK on success, S_FALSE on file error
 * @ingroup Loading
 */
HRESULT scan(const char *filename, RecordHandler handler, void *data, PROCESS mode);

/**
 * @brief Peek next character from file without consuming it
 * @param file FILE pointer to peek from
 * @return Next character, EOF if at end
 * @ingroup Loading
 */
int peek(FILE *file);

/**
 * @brief Format error string for missing parameters
 * @param error_buffer Buffer to write error message
 * @param error_flags Array indicating which fields are missing
 * @return S_OK on success, S_FALSE on buffer overflow
 * @ingroup Loading
 */
HRESULT format_basic_error_string(char *error_buffer, int *error_flags);

/**
 * @name Configuration Functions
 * @{
 */

/**
 * @brief Generate new configuration file interactively
 * @param configfile FILE pointer (can be NULL)
 * @param config_file Configuration file path
 * @return S_OK on success, S_FALSE on error
 * @ingroup Loading
 */
HRESULT generate_config(const char *config_file);

/**
 * @brief Recreate configuration file with new settings
 * @param config_file Configuration file path
 * @param mode Process mode
 * @return S_OK on success, error code on failure
 * @ingroup Loading
 */
HRESULT recreate_config(const char *config_file, PROCESS mode);

/**
 * @brief Read and apply existing configuration file
 * @param config_file Configuration file path
 * @return S_OK on success, S_FALSE if file doesn't exist
 * @ingroup Loading
 */
HRESULT read_config(const char *config_file);

/**
 * @brief Test configuration and create if missing
 * @param config_file Configuration file name
 * @param mode Process mode
 * @return S_OK on success, E_FAIL if user declines creation
 * @ingroup Loading
 */
HRESULT test_and_run_config(const char *config_file, PROCESS mode);

/**
 * @brief Handle configuration file record parsing
 * @param record Configuration record string
 * @param data User data (unused)
 * @param error_flags Error flags array
 * @return S_OK always (errors are non-fatal for config)
 * @ingroup Loading
 */
HRESULT handle_config(char *record, void *data, int *error_flags);

/** @} */

/**
 * @name Data File Handlers
 * @{
 */

/**
 * @brief Parse ammunition type record
 * @param record Ammunition record string
 * @param data Pointer to Ammo_t_List
 * @param error_flags Array to indicate missing fields
 * @return S_OK on success
 * @ingroup Loading
 */
HRESULT handle_ammo_t(char *record, void *data, int *error_flags);

/**
 * @brief Parse base record
 * @param record Base record string
 * @param data Pointer to Base_List
 * @param error_flags Array to indicate missing fields
 * @return S_OK on success
 * @ingroup Loading
 */
HRESULT handle_base(char *record, void *data, int *error_flags);

/**
 * @brief Parse target record
 * @param record Target record string
 * @param data Pointer to Target_List
 * @param error_flags Array to indicate missing fields
 * @return S_OK on success
 * @ingroup Loading
 */
HRESULT handle_target(char *record, void *data, int *error_flags);

/** @} */

/**
 * @brief Interactive parameter selection for calculations
 * 
 * Guides user through selecting base, target, and ammunition IDs.
 * Provides suggestions if invalid combinations are selected.
 * @return 0 on success, -1 to return to menu
 * @ingroup Loading
 */
int HandleParameterSelection(void);

/**
 * @brief Log calculation result to file
 * 
 * Appends detailed firing solution to log.txt in working directory.
 * @param base Pointer to firing base
 * @param tgt Pointer to target
 * @param ammo Pointer to ammunition type
 * @param res Pointer to ballistic result
 * @ingroup Loading
 */
void LogResult(const Base *base, const Target *tgt, const Ammo_t *ammo, const BallisticResult *res);

#endif /* LOAD_H */