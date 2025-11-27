#ifndef CALCULATIONS_H
#define CALCULATIONS_H

/// --- Coordinate ---
typedef struct {
    double latitude;
    double longitude;
    double altitude;
} Coordinate;


void SetupParameters();
int CalculateDistance(Coordinate coord1, Coordinate coord2, double *distance);
int ListBases();
int ListTargets();

#endif