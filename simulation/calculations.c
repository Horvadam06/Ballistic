/**
 * @file calculations.c
 * @brief Implementation of ballistic calculation functions
 *
 * This file contains the implementation of functions for calculating ballistic trajectories,
 * coordinate transformations, and optimal base/ammunition selection.
 *
 * @note Uses WGS84 ellipsoid model for Earth geometry and G7 drag coefficient model
 * @note Implements 4th-order Runge-Kutta integration for trajectory simulation
 * @note Distance tolerance: 1 meter, elevation search step: 0.1 degrees
 * @todo Add caching for frequently computed firing solutions
 * @todo Implement binary search for elevation angle optimization
 * @todo Add Coriolis effect for long-range trajectories
 * @todo Add wind effect modeling
 */

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>

#include "calculations.h"
#include "memory\load.h"
#include "thread\worker_platform.h"
#include "os\input_poll.h"
#include "thread\worker.h"
#include "helpers\helpers.h"
#include "helpers\color.h"

#define DEG2RAD (M_PI / 180.0) ///< Conversion factor from degrees to radians
#define RAD2DEG (180.0 / M_PI) ///< Conversion factor from radians to degrees

#define target_tolerance 100.0 // the distance, where the location becomes a target, and not a base

/**
 * @brief Square a value (macro for performance)
 *
 * @param x Value to square
 * @return Square of the input value
 */
#define sq(x) ((x) * (x))

// WGS84 parameters
static const double WGS84_A = 6378137.0;         ///< WGS84 semi-major axis (equatorial radius) in meters
static const double WGS84_E2 = 6.69437999014e-3; ///< WGS84 eccentricity squared

/**
 * @brief 3D vector structure for coordinate calculations
 */
typedef struct
{
    double x; ///< X coordinate (meters)
    double y; ///< Y coordinate (meters)
    double z; ///< Z coordinate (meters)
} Vec3;

/**
 * @brief Convert WGS84 coordinates to Earth-Centered, Earth-Fixed (ECEF) coordinates
 *
 * Transforms geographic coordinates (latitude, longitude, altitude) to Cartesian
 * coordinates in the ECEF reference frame.
 *
 * @param lat_deg Latitude in degrees
 * @param lon_deg Longitude in degrees
 * @param h Altitude above ellipsoid in meters
 * @return Vec3 containing ECEF coordinates (x, y, z) in meters
 */
static Vec3 wgs84_to_ecef(double lat_deg, double lon_deg, double h)
{
    double lat = lat_deg * DEG2RAD;
    double lon = lon_deg * DEG2RAD;

    double sinlat = sin(lat);
    double coslat = cos(lat);
    double sinlon = sin(lon);
    double coslon = cos(lon);

    double N = WGS84_A / sqrt(1 - WGS84_E2 * sinlat * sinlat);

    Vec3 r;
    r.x = (N + h) * coslat * coslon;
    r.y = (N + h) * coslat * sinlon;
    r.z = (N * (1 - WGS84_E2) + h) * sinlat;
    return r;
}

/**
 * @brief Convert ECEF coordinates to East-North-Up (ENU) coordinates
 *
 * Transforms from ECEF to local ENU coordinates with origin at specified location.
 *
 * @param p Point to transform (ECEF coordinates)
 * @param origin Origin point for ENU system (ECEF coordinates)
 * @param lat0_deg Origin latitude in degrees
 * @param lon0_deg Origin longitude in degrees
 * @return Vec3 containing ENU coordinates (x=east, y=north, z=up) in meters
 */
static Vec3 ecef_to_enu(Vec3 p, Vec3 origin, double lat0_deg, double lon0_deg)
{
    double lat0 = lat0_deg * DEG2RAD;
    double lon0 = lon0_deg * DEG2RAD;

    double sinlat = sin(lat0);
    double coslat = cos(lat0);
    double sinlon = sin(lon0);
    double coslon = cos(lon0);

    double dx = p.x - origin.x;
    double dy = p.y - origin.y;
    double dz = p.z - origin.z;

    Vec3 enu;
    enu.x = -sinlon * dx + coslon * dy;                                 // East
    enu.y = -sinlat * coslon * dx - sinlat * sinlon * dy + coslat * dz; // North
    enu.z = coslat * coslon * dx + coslat * sinlon * dy + sinlat * dz;  // Up
    return enu;
}

/**
 * @brief Calculate G7 drag coefficient based on velocity
 *
 * Approximates the G7 standard drag coefficient curve as a piecewise function
 * of Mach number. G7 is a common standard for long-range bullets.
 *
 * @param v Velocity in meters per second
 * @return Drag coefficient (dimensionless)
 */
static double g7_drag_coefficient(double v)
{
    // v is in m/s
    double mach = v / 343.0; // Speed of sound at sea level (approx)

    if (mach < 0.9)
        return 0.5;
    if (mach < 1.2)
        return 0.8;
    if (mach < 2.0)
        return 0.5;
    if (mach < 3.0)
        return 0.3;
    return 0.2;
}

#define G 9.80665         ///< Gravitational acceleration (m/s²)
#define AIR_DENSITY 1.225 ///< Air density at sea level (kg/m³)

/**
 * @brief State vector for ballistic trajectory simulation
 */
typedef struct
{
    double x, y, z;    ///< Position coordinates (meters)
    double vx, vy, vz; ///< Velocity components (m/s)
} State;

/**
 * @brief Calculate derivatives for ballistic motion equations
 *
 * Computes the rate of change of position and velocity based on
 * gravity and aerodynamic drag using the G7 drag model.
 *
 * @param s Current state vector
 * @param d Output derivatives (dx/dt, dy/dt, etc.)
 * @param BC Ballistic coefficient (kg/m²)
 */
static void derivatives(const State *s, State *d, double BC)
{
    double v = sqrt(sq(s->vx) + sq(s->vy) + sq(s->vz));
    if (v < 1e-6)
        v = 1e-6; // avoid divide by zero
    double Cd = g7_drag_coefficient(v);
    double drag = 0.5 * AIR_DENSITY * Cd * sq(v) / BC;

    d->x = s->vx;
    d->y = s->vy;
    d->z = s->vz;

    d->vx = -drag * s->vx / v;
    d->vy = -drag * s->vy / v;
    d->vz = -G - drag * s->vz / v;
}

/**
 * @brief Perform one step of 4th-order Runge-Kutta integration
 *
 * Advances the state vector by time dt using RK4 integration method.
 *
 * @param s State vector to update (input/output)
 * @param dt Time step (seconds)
 * @param BC Ballistic coefficient (kg/m²)
 */
static void rk4(State *s, double dt, double BC)
{
    State k1, k2, k3, k4, tmp;

    derivatives(s, &k1, BC);

    tmp = *s;
    tmp.x += 0.5 * dt * k1.x;
    tmp.y += 0.5 * dt * k1.y;
    tmp.z += 0.5 * dt * k1.z;
    tmp.vx += 0.5 * dt * k1.vx;
    tmp.vy += 0.5 * dt * k1.vy;
    tmp.vz += 0.5 * dt * k1.vz;
    derivatives(&tmp, &k2, BC);

    tmp = *s;
    tmp.x += 0.5 * dt * k2.x;
    tmp.y += 0.5 * dt * k2.y;
    tmp.z += 0.5 * dt * k2.z;
    tmp.vx += 0.5 * dt * k2.vx;
    tmp.vy += 0.5 * dt * k2.vy;
    tmp.vz += 0.5 * dt * k2.vz;
    derivatives(&tmp, &k3, BC);

    tmp = *s;
    tmp.x += dt * k3.x;
    tmp.y += dt * k3.y;
    tmp.z += dt * k3.z;
    tmp.vx += dt * k3.vx;
    tmp.vy += dt * k3.vy;
    tmp.vz += dt * k3.vz;
    derivatives(&tmp, &k4, BC);

    s->x += (dt / 6.0) * (k1.x + 2 * k2.x + 2 * k3.x + k4.x);
    s->y += (dt / 6.0) * (k1.y + 2 * k2.y + 2 * k3.y + k4.y);
    s->z += (dt / 6.0) * (k1.z + 2 * k2.z + 2 * k3.z + k4.z);

    s->vx += (dt / 6.0) * (k1.vx + 2 * k2.vx + 2 * k3.vx + k4.vx);
    s->vy += (dt / 6.0) * (k1.vy + 2 * k2.vy + 2 * k3.vy + k4.vy);
    s->vz += (dt / 6.0) * (k1.vz + 2 * k2.vz + 2 * k3.vz + k4.vz);
}

/**
 * @brief Solve ballistic trajectory for given target and parameters
 *
 * Iteratively searches for the optimal elevation angle to hit a target
 * using numerical simulation of the projectile trajectory.
 *
 * @param targetENU Target position in ENU coordinates relative to shooter
 * @param muzzleVel Muzzle velocity (m/s)
 * @param BC Ballistic coefficient (kg/m²)
 * @return BallisticResult containing firing solution parameters
 *
 * @note Searches elevation angles from 5 to 80 degrees with 0.1 degree steps
 * @note Uses 1 meter tolerance for altitude matching
 * @todo Replace linear search with binary search for better performance
 */
static BallisticResult solve_ballistic(Vec3 targetENU, double muzzleVel, double BC, const WorkerContext *ctx)
{
    BallisticResult r = {0};
    double azimuth = atan2(targetENU.x, targetENU.y); // east-north convention
    r.azimuth_deg = azimuth * RAD2DEG;

    double R = sqrt(targetENU.x * targetENU.x + targetENU.y * targetENU.y);
    double DZ = targetENU.z;

    double best_error = DBL_MAX;

    // Search through elevation angles from 5 to 80 degrees
    for (double elev_deg = 5.0; elev_deg <= 80.0; elev_deg += 0.1)
    {
        if (ctx && atomic_load(&ctx->state) == WORKER_CANCELLED)
            return (BallisticResult){.valid = -1};

        double elev = elev_deg * DEG2RAD;

        State s = {0};
        s.x = s.y = s.z = 0;
        s.vx = muzzleVel * cos(elev) * sin(azimuth);
        s.vy = muzzleVel * cos(elev) * cos(azimuth);
        s.vz = muzzleVel * sin(elev);

        double t = 0.0;
        double dt = 0.02; // 20ms time step

        // Simulate trajectory
        for (;;)
        {
            if (ctx && atomic_load(&ctx->state) == WORKER_CANCELLED)
                return (BallisticResult){.valid = -1};

            rk4(&s, dt, BC);
            t += dt;

            if (s.z < DZ - 50.0)
                break; // too low too early (miss)
            if (t > 200.0)
                break; // safety breaker (max 200 seconds flight time)

            // Check if projectile is near target altitude (1 meter tolerance)
            if (s.z < DZ && s.z > DZ - 1.0)
            {
                double dist = sqrt(s.x * s.x + s.y * s.y);
                double error = fabs(dist - R);

                if (error < best_error)
                {
                    best_error = error;
                    r.elevation_deg = elev_deg;
                    r.time_of_flight = t;
                    r.impact_velocity = sqrt(s.vx * s.vx + s.vy * s.vy + s.vz * s.vz);
                    r.valid = 1;
                }
                break;
            }
        }
    }

    return r;
}

/**
 * @brief Computes a firing solution for a given base, target, and ammunition
 *
 * Main function that orchestrates coordinate transformations and trajectory
 * calculations to determine firing parameters.
 *
 * @param b Pointer to the firing base structure
 * @param t Pointer to the target structure
 * @param ammo Pointer to the ammunition type structure
 * @return BallisticResult structure containing all calculated firing parameters
 *
 * @see BallisticResult
 * @note Firing solution is invalid if no elevation angle produces a hit within tolerance
 * @todo Cache results for frequently used base-target-ammo combinations
 */
BallisticResult compute_fire_solution(Base *b, Target *t, Ammo_t *ammo, const WorkerContext *ctx)
{
    Vec3 shooterECEF = wgs84_to_ecef(b->coord.latitude, b->coord.longitude, b->coord.altitude);
    Vec3 targetECEF = wgs84_to_ecef(t->coord.latitude, t->coord.longitude, t->coord.altitude);

    Vec3 enu_target = ecef_to_enu(
        targetECEF,
        shooterECEF,
        b->coord.latitude,
        b->coord.longitude);

    return solve_ballistic(enu_target, ammo->muzzleVelocity, ammo->ballistic_coefficient, ctx);
}

// ============================================================
// Helper functions for base suggestion algorithms
// ============================================================

/**
 * @brief Calculate 2D great-circle distance between two coordinates
 *
 * Uses Haversine formula to compute distance along Earth's surface,
 * ignoring altitude differences.
 *
 * @param a First coordinate
 * @param b Second coordinate
 * @return Distance in meters
 */
static double GeoDistance2D(const Coordinate *a, const Coordinate *b)
{
    if (!a || !b)
        return 0.0;

    double lat1 = a->latitude * DEG2RAD;
    double lat2 = b->latitude * DEG2RAD;
    double dlat = lat2 - lat1;
    double dlon = (b->longitude - a->longitude) * DEG2RAD;

    double s1 = sin(dlat * 0.5);
    double s2 = sin(dlon * 0.5);

    double h = s1 * s1 + cos(lat1) * cos(lat2) * s2 * s2;
    double c = 2.0 * atan2(sqrt(h), sqrt(1.0 - h));
    return WGS84_A * c; // meters (approx mean Earth radius via WGS84 a)
}

/**
 * @brief Calculate 3D Euclidean distance between two coordinates
 *
 * Combines 2D surface distance with altitude difference to get
 * straight-line 3D distance.
 *
 * @param a First coordinate
 * @param b Second coordinate
 * @return 3D distance in meters
 */
static double GeoDistance3D(const Coordinate *a, const Coordinate *b)
{
    double d2d = GeoDistance2D(a, b);
    double dz = (b->altitude - a->altitude);
    return sqrt(d2d * d2d + dz * dz);
}

/**
 * @brief Check if a base has available ammunition of a specific type
 *
 * @param b Base to check
 * @param ammo_id Ammunition type ID to look for
 * @return 1 if base has the ammunition with quantity > 0, 0 otherwise
 */
int BaseHasAmmo(Base *b, unsigned ammo_id)
{
    if (!b || !b->ammoStock)
        return 0;

    Ammo_Stock *it = b->ammoStock->head;
    while (it)
    {
        if (it->typePtr && it->typePtr->id == ammo_id && it->quantity > 0)
            return 1;
        it = it->next;
    }
    return 0;
}

/**
 * @brief Suggests the best base and ammunition combination for a target
 *
 * Evaluates all bases and their available ammunition to find the optimal
 * combination based on:
 * 1. Closest distance to target (primary criterion)
 * 2. Ammunition quantity (tie-breaker)
 *
 * @param list Pointer to the list of available bases
 * @param tgt Pointer to the target structure
 * @param outBase Double pointer to return the selected base (output parameter)
 * @param outAmmo Double pointer to return the selected ammunition (output parameter)
 * @return 1 if a valid combination was found, 0 otherwise
 *
 * @note Only considers ammunition with valid firing solutions
 * @note Uses 1 meter tolerance for distance comparisons
 * @note O(n*m) complexity where n = number of bases, m = ammunition types per base
 * @todo Implement early exit when distance is significantly worse than current best
 * @todo Add weight factors for different ammunition types (lethality, cost, etc.)
 */
/*int SuggestBestBaseAndAmmo(Base_List *list, Target *tgt, Base **outBase, Ammo_t **outAmmo, const WorkerContext *ctx)
{
    if (!list || !tgt || !outBase || !outAmmo)
        return 0;

    Base *bestBase = NULL;
    Ammo_t *bestAmmo = NULL;
    double bestDist = DBL_MAX;
    unsigned bestQty = 0; // tie-breaker: more stock wins

    Base_Node *bn = list->head;
    while (bn)
    {
        if (ctx && atomic_load(&ctx->state) == WORKER_CANCELLED)
            return -1;

        Base *b = bn->baseItem;
        bn = bn->next;

        if (!b || !b->ammoStock || !b->ammoStock->head)
            continue;

        double dist3d = GeoDistance3D(&b->coord, &tgt->coord);

        // Early rejection: if already farther than best candidate + 1 meter tolerance, skip
        if (dist3d > bestDist + 1.0 || dist3d < target_tolerance)
            continue;

        Ammo_Stock *stock = b->ammoStock->head;
        while (stock)
        {
            if (ctx && atomic_load(&ctx->state) == WORKER_CANCELLED)
                return -1;

            Ammo_t *a = stock->typePtr;

            if (a && stock->quantity > 0)
            {
                BallisticResult sol = compute_fire_solution(b, tgt, a, ctx);
                if (sol.valid)
                {
                    int better = 0;

                    if (dist3d < bestDist - 1.0)
                    {
                        // Strictly closer (by more than 1 meter) is always better
                        better = 1;
                    }
                    else if (fabs(dist3d - bestDist) <= 1.0)
                    {
                        // If same distance within 1 meter tolerance, ammunition quantity decides
                        if (stock->quantity > bestQty)
                            better = 1;
                    }

                    if (better)
                    {
                        bestDist = dist3d;
                        bestQty = stock->quantity;
                        bestBase = b;
                        bestAmmo = a;
                    }
                }
            }
            stock = stock->next;
        }
    }

    if (!bestBase)
        return 0;

    *outBase = bestBase;
    *outAmmo = bestAmmo;
    return 1;
}
*/
/**
 * @brief Suggests the best base and ammunition combination with regional awareness
 *
 * Similar to SuggestBestBaseAndAmmo but prioritizes bases in the same region
 * as the target. Region is determined by ID ranges (1000-based blocks).
 *
 * Algorithm:
 * 1. First pass: only consider bases in same region as target
 * 2. If no solution found in same region, second pass: consider all bases
 *
 * @param list Pointer to the list of available bases
 * @param tgt Pointer to the target structure
 * @param outBase Double pointer to return the selected base (output parameter)
 * @param outAmmo Double pointer to return the selected ammunition (output parameter)
 * @return 1 if a valid combination was found, 0 otherwise
 *
 * @note Region is determined by ID: target.id/1000 defines region block
 * @note Uses 1 meter tolerance for distance comparisons
 * @todo Make region calculation configurable (not hardcoded to 1000-based blocks)
 * @todo Add region weighting factor (e.g., prefer same region but allow cross-region if much better)
 */

// later update it, so it precalculates every ammo region with a tolerance
// and then only check the options in the calcuated area, making it faster and cheaper, also there is a chance
// that it skips a valid option, because of height difference etc.
// so the best option would be a fast processing, and slow processing, where at fast it only compares values, and does one or so calculations
// and slow sort checks all the options like the current version
int SuggestBestBaseAndAmmo_RegionAware(Base_List *list, Target *tgt, Base **outBase, Ammo_t **outAmmo, WorkerContext *ctx)
{
    // TODO
    // remove any comments, and move them to the caller, no stdout in the worker

    if (!list || !tgt || !outBase || !outAmmo)
        return 0;

    unsigned regionStart = (tgt->id / 1000) * 1000;
    unsigned regionEnd = regionStart + 999;

    Base *bestBase = NULL;
    Ammo_t *bestAmmo = NULL;
    double bestDist = DBL_MAX;
    unsigned bestQty = 0;

    // printf("Testing base...:\n");

    int pass;
    for (pass = 1; pass <= 2; pass++)
    {
        Base_Node *bn = list->head;

        while (bn)
        {
            if (ctx && atomic_load(&ctx->state) == WORKER_CANCELLED)
                return -1;

            Base *b = bn->baseItem;
            bn = bn->next;

            if (!b || !b->ammoStock || !b->ammoStock->head) // test b->id
                continue;

            int inRegion = (b->id >= regionStart && b->id <= regionEnd);

            if (pass == 1 && !inRegion)
                continue; // first pass: region only
            if (pass == 2 && inRegion)
                continue; // second pass: everything except region

            double dist3d = GeoDistance3D(&b->coord, &tgt->coord);

            // if the base distnace is better, then before continue the search
            if (dist3d > bestDist + 1.0)
            {
                continue;
            }

            atomic_store(&ctx->current_base_id, b->id);
            /*printf("\tid: %5u | label: %20s | ",
                   b->id, b->label);*/

            // if the distance is smaller, than 100m so it's probably the same geo location, then continue
            if (dist3d < target_tolerance)
            {
                // printf("[Too close to the target!]\n");
                continue;
            }

            atomic_store(&ctx->work_total, get_base_ammo_t_count(b));
            atomic_store(&ctx->work_done, 0);

            Ammo_Stock *stock = b->ammoStock->head;
            while (stock)
            {
                if (ctx && atomic_load(&ctx->state) == WORKER_CANCELLED)
                    return -1;

                Ammo_t *a = stock->typePtr;

                if (a && stock->quantity > 0)
                {
                    atomic_store(&ctx->current_ammo_id, a->id);
                    BallisticResult sol = compute_fire_solution(b, tgt, a, ctx);
                    if (sol.valid)
                    {
                        int better = 0;

                        if (dist3d < bestDist - 1.0)
                            better = 1;
                        else if (fabs(dist3d - bestDist) <= 1.0)
                        {
                            if (stock->quantity > bestQty)
                                better = 1;
                        }

                        if (better)
                        {
                            bestBase = b;
                            bestAmmo = a;
                            bestDist = dist3d;
                            bestQty = stock->quantity;
                        }
                    }
                }
                atomic_fetch_add(&ctx->work_done, 1);

                stock = stock->next;
            }
            // printf("[Searching...]\n");
        }

        // If pass 1 found something, stop here (prefer regional solutions)
        if (bestBase)
            break;
    }

    // printf("Finished!\n\n");

    if (!bestBase)
        return 0;

    *outBase = bestBase;
    *outAmmo = bestAmmo;
    return 1;
}

static void hook_status_before(void *arg)
{
    if (!arg)
        return;
    WorkerContext *ctx = (WorkerContext *)arg;
    add_seperator_s(NULL, NULL, NULL, HLINE, NORMAL_SEPERATOR_SIZE);
    printf(" - base id: %5u | ammo id: %5u \n - progress: ", ctx->current_base_id, ctx->current_ammo_id);
}

static void hook_status_after(void *arg)
{
    (void)arg;
    printf("\n");
    add_seperator_s(NULL, NULL,NULL, HLINE, NORMAL_SEPERATOR_SIZE);
}

int SuggestBestBaseAndAmmo_Parallel(Base_List *list, Target *tgt, Base **outBase, Ammo_t **outAmmo)
{
    WorkerContext ctx = {0};
    ctx.list = list;
    ctx.target = tgt;

    atomic_store(&ctx.work_total, 0);
    atomic_store(&ctx.work_done, 0);
    atomic_store(&ctx.state, WORKER_IDLE);

    // start the worker thread
    thread_t t = start_worker(&ctx);

    input_init();
    HIDE_CURSOR();

    unsigned old_percent = 0;
    while (1)
    {
        unsigned cur_percent = progress_percent(atomic_load(&ctx.work_done), atomic_load(&ctx.work_total));

        if (cur_percent != old_percent)
        {
            render_progress_bar(
                cur_percent,
                hook_status_before,
                hook_status_after, &ctx, NULL);

            printf("\033[%dA", 4);
        }

        old_percent = cur_percent;

        if (input_available())
        {

            int c = tolower(input_getchar());
            if (c == 'q' || c == '\e')
            {
                // signal cancellation
                atomic_store(&ctx.state, WORKER_CANCELLED);
            }
        }

        WorkerState s = atomic_load(&ctx.state);
        if (s == WORKER_DONE || s == WORKER_FAILED || s == WORKER_CANCELLED)
            break;

        Sleep(20); // don't busy-wait
    }

    printf("\033[%dB", 4);
    input_shutdown();
    SHOW_CURSOR();
    printf("\n");

    // wait for thread to exit cleanly
    join_worker(t);

    // handle result
    WorkerState final_state = atomic_load(&ctx.state);
    if (final_state == WORKER_DONE)
    {
        *outBase = ctx.result_base; // assuming you filled ctx.base in worker
        *outAmmo = ctx.result_ammo;

        return 1; // success
    }
    else if (final_state == WORKER_CANCELLED)
    {
        printf("Cancelled by user.\n");
        return 0;
    }
    else
    {
        printf("Worker failed.\n");
        return 0;
    }
}