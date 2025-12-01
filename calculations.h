#ifndef CALCULATIONS_H
#define CALCULATIONS_H

#include "storage.h"

/// --- Coordinate ---

void SetupParameters();
int CalculateDistance(Coordinate coord1, Coordinate coord2, double *distance);

typedef struct
{
    double elevation_deg;
    double azimuth_deg;
    double time_of_flight;
    double impact_velocity;
    int valid;
} BallisticResult;

BallisticResult compute_fire_solution(Base *b, Target *t, Ammo_t *ammo);
int SuggestBestBaseAndAmmo(Base_List *list, Target *tgt, Base **outBase, Ammo_t **outAmmo);
int SuggestBestBaseAndAmmo_RegionAware(Base_List *list, Target *tgt, Base **outBase, Ammo_t **outAmmo);
// int ListBases();
// int ListTargets();

#endif