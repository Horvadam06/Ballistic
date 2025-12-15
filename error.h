/**
 * @defgroup Utilities Utility Functions
 * @brief Error handling, color management, helper functions, and debugging
 * 
 * This module contains utility functions used throughout the system.
 */

/**
 * @file error.h
 * @brief Error codes and process states
 * @ingroup Utilities
 */

#ifndef ERROR_H
#define ERROR_H

/**
 * @brief Result codes (similar to Windows P_HRESULT)
 * @ingroup Utilities
 */
typedef enum
{
    P_S_OK = 0,     ///< Operation successful
    P_S_FALSE = 1,  ///< Operation completed but with non-fatal issue
    P_S_PARTIAL = 2, ///< Operation partially completed (missing parameters)
    P_E_FAIL = -1   ///< Fatal error occurred
} P_HRESULT;      //aka private hresult

/**
 * @brief Program process states
 * @ingroup Utilities
 */
typedef enum
{
    S_RUNNING,    ///< Program is running normally
    S_END,        ///< Program is ending gracefully
    S_TERMINATE,  ///< Program is terminating due to error
    S_DEBUG       ///< Program is in debug mode
} PROCESS;

/**
 * @brief Check if result indicates success
 * @param hr Result code to check
 * @return 1 if success (hr >= 0), 0 otherwise
 * @ingroup Utilities
 */
int P_SUCCEEDED(P_HRESULT hr);

/**
 * @brief Check if result indicates failure
 * @param hr Result code to check
 * @return 1 if failure (hr < 0), 0 otherwise
 * @ingroup Utilities
 */
int P_FAILED(P_HRESULT hr);

#endif /* ERROR_H */