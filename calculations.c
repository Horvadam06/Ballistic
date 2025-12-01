#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>

#include "calculations.h"
#include "load.h"

#include "debugmalloc.h"

#define DEG2RAD (M_PI / 180.0)
#define RAD2DEG (180.0 / M_PI)

#define sq(x) ((x) * (x))

// WGS84 parameters
static const double WGS84_A = 6378137.0;         // major axis
static const double WGS84_E2 = 6.69437999014e-3; // eccentricity^2

typedef struct
{
    double x, y, z;
} Vec3;

// WGS84 to Earth Centered, Earth Fixed
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

// Convert ECEF to ENU with origin at (lat0, lon0)
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

static double g7_drag_coefficient(double v)
{
    // v is in m/s
    double mach = v / 343.0;

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

#define G 9.80665
#define AIR_DENSITY 1.225

typedef struct
{
    double x, y, z;    // position (meters)
    double vx, vy, vz; // velocity (m/s)
} State;

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

static BallisticResult solve_ballistic(Vec3 targetENU, double muzzleVel, double BC)
{
    BallisticResult r = {0};
    double azimuth = atan2(targetENU.x, targetENU.y); // east-north convention
    r.azimuth_deg = azimuth * RAD2DEG;

    double R = sqrt(targetENU.x * targetENU.x + targetENU.y * targetENU.y);
    double DZ = targetENU.z;

    double best_error = DBL_MAX;

    for (double elev_deg = 5.0; elev_deg <= 80.0; elev_deg += 0.1)
    {
        double elev = elev_deg * DEG2RAD;

        State s = {0};
        s.x = s.y = s.z = 0;
        s.vx = muzzleVel * cos(elev) * sin(azimuth);
        s.vy = muzzleVel * cos(elev) * cos(azimuth);
        s.vz = muzzleVel * sin(elev);

        double t = 0.0;
        double dt = 0.02;

        for (;;)
        {
            rk4(&s, dt, BC);
            t += dt;

            if (s.z < DZ - 50.0)
                break; // too low too early (miss)
            if (t > 200.0)
                break; // safety breaker
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

BallisticResult compute_fire_solution(Base *b, Target *t, Ammo_t *ammo)
{
    Vec3 shooterECEF = wgs84_to_ecef(b->coord.latitude, b->coord.longitude, b->coord.altitude);
    Vec3 targetECEF = wgs84_to_ecef(t->coord.latitude, t->coord.longitude, t->coord.altitude);

    Vec3 enu_target = ecef_to_enu(
        targetECEF,
        shooterECEF,
        b->coord.latitude,
        b->coord.longitude);

    return solve_ballistic(enu_target, ammo->muzzleVelocity, ammo->ballistic_coefficient);
}

// For the base suggestion================================
//  Helper: Haversine 2D distance
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

// Helper: 3D geo distance (meters)
static double GeoDistance3D(const Coordinate *a, const Coordinate *b)
{
    double d2d = GeoDistance2D(a, b);
    double dz = (b->altitude - a->altitude);
    return sqrt(d2d * d2d + dz * dz);
}

// Helper: check if base has ammo (quantity > 0)
static int BaseHasAmmo(Base *b, unsigned ammo_id)
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

// SuggestBestBase
int SuggestBestBaseAndAmmo(Base_List *list, Target *tgt, Base **outBase, Ammo_t **outAmmo)
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
        Base *b = bn->baseItem;
        bn = bn->next;

        if (!b || !b->ammoStock || !b->ammoStock->head)
            continue;

        double dist3d = GeoDistance3D(&b->coord, &tgt->coord);

        // hard reject if already farther than a confirmed better candidate
        if (dist3d > bestDist + 0.0001)
            continue;

        Ammo_Stock *stock = b->ammoStock->head;
        while (stock)
        {
            Ammo_t *a = stock->typePtr;

            if (a && stock->quantity > 0)
            {
                BallisticResult sol = compute_fire_solution(b, tgt, a);
                if (sol.valid)
                {
                    int better = 0;

                    if (dist3d < bestDist - 0.0001)
                    {
                        // strictly closer is always better
                        better = 1;
                    }
                    else if (fabs(dist3d - bestDist) <= 0.0001)
                    {
                        // if same distance, then ammo quantity decides
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

int SuggestBestBaseAndAmmo_RegionAware(Base_List *list, Target *tgt, Base **outBase, Ammo_t **outAmmo)
{
    if (!list || !tgt || !outBase || !outAmmo)
        return 0;

    unsigned regionStart = (tgt->id / 1000) * 1000;
    unsigned regionEnd = regionStart + 999;

    Base *bestBase = NULL;
    Ammo_t *bestAmmo = NULL;
    double bestDist = DBL_MAX;
    unsigned bestQty = 0;

    int pass;
    for (pass = 1; pass <= 2; pass++)
    {
        Base_Node *bn = list->head;

        while (bn)
        {
            Base *b = bn->baseItem;
            bn = bn->next;

            if (!b || !b->ammoStock || !b->ammoStock->head)
                continue;

            int inRegion = (b->id >= regionStart && b->id <= regionEnd);

            if (pass == 1 && !inRegion)
                continue; // first pass: region only
            if (pass == 2 && inRegion)
                continue; // second pass: everything except region

            double dist3d = GeoDistance3D(&b->coord, &tgt->coord);

            if (dist3d > bestDist + 0.0001)
                continue;

            Ammo_Stock *stock = b->ammoStock->head;
            while (stock)
            {
                Ammo_t *a = stock->typePtr;

                if (a && stock->quantity > 0)
                {
                    BallisticResult sol = compute_fire_solution(b, tgt, a);
                    if (sol.valid)
                    {
                        int better = 0;

                        if (dist3d < bestDist - 0.0001)
                            better = 1;
                        else if (fabs(dist3d - bestDist) <= 0.0001)
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

                stock = stock->next;
            }
        }

        // If pass 1 found something, stop here.
        if (bestBase)
            break;
    }

    if (!bestBase)
        return 0;

    *outBase = bestBase;
    *outAmmo = bestAmmo;
    return 1;
}
