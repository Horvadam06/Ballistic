#ifndef CALCULATIONS_H
#define CALCULATIONS_H

typedef struct
{
    double weight;
    double diameter;
    double blastRadius;
} ammunition;

typedef struct
{
    double latitude;  // latitide or phi
    double longitude; // longitude
    double altitude;  // distance from sea level
} coordinate;


void SetupParameters();
int CalculateDistance(coordinate coord1, coordinate coord2, double *distance);
int ListBases();
int ListTargets();

#endif