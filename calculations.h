/**

@file calculations.h

@brief Header file for ballistic calculation functions and structures

This file contains declarations for ballistic computations and related data structures

used to calculate firing solutions and recommend optimal base/ammunition combinations.
*/

#ifndef CALCULATIONS_H
#define CALCULATIONS_H

#include "storage.h"

/**

@brief Structure representing the result of ballistic calculations

Contains all computed parameters needed for firing at a target including

elevation, azimuth, time of flight, and impact velocity.
*/
typedef struct
{
    double elevation_deg;   ///< Required elevation angle in degrees
    double azimuth_deg;     ///< Required azimuth angle in degrees
    double time_of_flight;  ///< Estimated time of flight in seconds
    double impact_velocity; ///< Estimated impact velocity in m/s
    int valid;              ///< Flag indicating if the solution is valid (1 = valid, 0 = invalid)
} BallisticResult;

/**

@brief Computes a firing solution for a given base, target, and ammunition

Calculates the required elevation and azimuth angles, time of flight, and impact

velocity for hitting a target from a specific firing base with given ammunition.

@param b Pointer to the firing base structure

@param t Pointer to the target structure

@param ammo Pointer to the ammunition type structure

@return BallisticResult structure containing all calculated firing parameters
*/
BallisticResult compute_fire_solution(Base *b, Target *t, Ammo_t *ammo);

/**

@brief Suggests the best base and ammunition combination for a target

Analyzes all available bases and ammunition types to find the optimal

combination for engaging the specified target based on ballistic performance.

@param list Pointer to the list of available bases

@param tgt Pointer to the target structure

@param outBase Double pointer to return the selected base (output parameter)

@param outAmmo Double pointer to return the selected ammunition (output parameter)

@return 1 if a valid combination was found, 0 otherwise
*/
int SuggestBestBaseAndAmmo(Base_List *list, Target *tgt, Base **outBase, Ammo_t **outAmmo);

/**

@brief Suggests the best base and ammunition combination with regional awareness

Similar to SuggestBestBaseAndAmmo but takes into account regional constraints

or preferences when selecting the optimal base-ammunition combination.

@param list Pointer to the list of available bases

@param tgt Pointer to the target structure

@param outBase Double pointer to return the selected base (output parameter)

@param outAmmo Double pointer to return the selected ammunition (output parameter)

@return 1 if a valid combination was found, 0 otherwise
*/
int SuggestBestBaseAndAmmo_RegionAware(Base_List *list, Target *tgt, Base **outBase, Ammo_t **outAmmo);

#endif

/** @} */