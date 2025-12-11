/**
 * @file main.h
 * @brief Main program declarations
 * @defgroup Main Main Program
 * @brief Program entry point and lifecycle management
 */

#ifndef MAIN_H
#define MAIN_H

#include "error.h"

/**
 * @brief Program initialization and setup
 * 
 * Performs all startup tasks:
 * 1. Loads or creates configuration
 * 2. Runs system tests
 * 3. Loads data files (ammo, bases, targets)
 * 4. Displays welcome message
 * 5. Shows help
 * 
 * @return Initial process state (S_RUNNING on success, S_TERMINATE on failure)
 * @ingroup Main
 */
PROCESS Setup();

#endif /* MAIN_H */