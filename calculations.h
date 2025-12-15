/**
 * @defgroup Calculations Ballistic Calculations
 * @brief Ballistic trajectory simulation and coordinate transformations
 *
 * This module performs all ballistic calculations including:
 * - WGS84 to ECEF coordinate transformations
 * - ECEF to local ENU coordinate transformations
 * - G7 drag coefficient modeling
 * - 4th-order Runge-Kutta trajectory simulation
 * - Optimal base and ammunition selection algorithms
 */

#ifndef CALCULATIONS_H
#define CALCULATIONS_H

#include "storage.h"

typedef struct WorkerContext WorkerContext;

/**
 * @brief Ballistic calculation results
 *
 * Contains all computed parameters for a firing solution.
 * @ingroup Calculations
 */
typedef struct
{
    double elevation_deg;   ///< Required elevation angle in degrees (5-80° range)
    double azimuth_deg;     ///< Required azimuth angle in degrees (0-360°)
    double time_of_flight;  ///< Estimated time of flight in seconds
    double impact_velocity; ///< Estimated impact velocity in m/s
    int valid;              ///< Flag indicating valid solution (1) or invalid (0)
} BallisticResult;

/**
 * @brief Compute firing solution for base-target-ammunition combination
 *
 * Main ballistic calculation function. Performs coordinate transformations
 * and trajectory simulation to determine firing parameters.
 *
 * @param b Pointer to firing base
 * @param t Pointer to target
 * @param ammo Pointer to ammunition type
 * @return BallisticResult structure with all calculated parameters
 *
 * @note Uses 1 meter tolerance for altitude matching
 * @note Searches elevation angles from 5° to 80° in 0.1° increments
 * @note Maximum simulated flight time: 200 seconds
 *
 * @ingroup Calculations
 */
BallisticResult compute_fire_solution(Base *b, Target *t, Ammo_t *ammo, const WorkerContext *ctx);

/**
 * @brief Suggest optimal base and ammunition for a target
 *
 * Evaluates all bases and ammunition combinations to find the best match
 * based on distance and ammunition availability.
 *
 * Selection criteria (in order):
 * 1. Valid firing solution exists
 * 2. Minimum 3D distance to target (primary)
 * 3. Maximum ammunition quantity (tie-breaker)
 *
 * @param list Pointer to base list
 * @param tgt Pointer to target
 * @param outBase Double pointer to receive selected base
 * @param outAmmo Double pointer to receive selected ammunition
 * @return 1 if valid combination found, 0 otherwise
 *
 * @note Uses 1 meter tolerance for distance comparisons
 * @note Complexity: O(n×m) where n=bases, m=ammunition types per base
 *
 * @ingroup Calculations
 */
//int SuggestBestBaseAndAmmo(Base_List *list, Target *tgt, Base **outBase, Ammo_t **outAmmo, const WorkerContext *ctx);

/**
 * @brief Suggest optimal base and ammunition with regional preference
 *
 * Similar to SuggestBestBaseAndAmmo but prefers bases in the same
 * region as the target. Region is determined by ID blocks of 1000.
 *
 * Algorithm:
 * 1. First pass: Only consider bases with IDs in same 1000-block as target
 * 2. If no solution in same region, second pass: Consider all bases
 *
 * @param list Pointer to base list
 * @param tgt Pointer to target
 * @param outBase Double pointer to receive selected base
 * @param outAmmo Double pointer to receive selected ammunition
 * @return 1 if valid combination found, 0 otherwise
 *
 * @note Region: target.id/1000 determines the 1000-based block
 * @note Uses same selection criteria as SuggestBestBaseAndAmmo
 *
 * @ingroup Calculations
 */
int SuggestBestBaseAndAmmo_RegionAware(Base_List *list, Target *tgt, Base **outBase, Ammo_t **outAmmo, WorkerContext *ctx);

int SuggestBestBaseAndAmmo_Parallel(Base_List *list, Target *tgt, Base **outBase, Ammo_t **outAmmo);

#endif /* CALCULATIONS_H */