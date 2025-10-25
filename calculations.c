#include "calculations.h"
#include "storage.h"
#include "load.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define sq(x) ((x) * (x))

double DegToRad(double deg){
    return deg*M_PI/180.0L;
}

int CalculateDistance(coordinate coord1, coordinate coord2, double *distance)
{
    // WGS-84 constants
    double a = 6378137.0;
    double f = 1.0 / 298.257223563;
    double b = (1.0 - f) * a;

    // convert degrees to radians
    coord1.latitude = DegToRad(coord1.latitude);
    coord2.latitude = DegToRad(coord2.latitude);
    coord1.longitude = DegToRad(coord1.longitude);
    coord2.longitude = DegToRad(coord2.longitude);
    
    double U1 = atan((1.0 - f) * tan(coord1.latitude));
    double U2 = atan((1.0 - f) * tan(coord2.latitude));
    double L = coord2.longitude - coord1.longitude;

    double lambda = L, prev_lambda;
    double sinU1, cosU1, sinU2, cosU2;
    double sinSigma, cosSigma, sigma, sinAlpha, cosSqAlpha, cos2Sigma_m;

    int iter = 0;
    do {
        prev_lambda = lambda;

        sinU1 = sin(U1); cosU1 = cos(U1);
        sinU2 = sin(U2); cosU2 = cos(U2);

        double sinLambda = sin(lambda);
        double cosLambda = cos(lambda);

        sinSigma = sqrt(sq(cosU2 * sinLambda) +
                        sq(cosU1 * sinU2 - sinU1 * cosU2 * cosLambda));

        cosSigma = sinU1 * sinU2 + cosU1 * cosU2 * cosLambda;
        sigma = atan2(sinSigma, cosSigma);

        sinAlpha = (cosU1 * cosU2 * sinLambda) / sinSigma;
        cosSqAlpha = 1.0 - sq(sinAlpha);

        cos2Sigma_m = cosSigma - (2.0 * sinU1 * sinU2) / cosSqAlpha;
        double C = (f / 16.0) * cosSqAlpha * (4.0 + f * (4.0 - 3.0 * cosSqAlpha));

        lambda = L + (1.0 - C) * f * sinAlpha *
            (sigma + C * sinSigma * (cos2Sigma_m + C * cosSigma * (-1.0 + 2.0 * sq(cos2Sigma_m))));

        iter++;
    } while (fabs(lambda - prev_lambda) > 1e-12 && iter < 100);

    // Post-convergence
    double usq = cosSqAlpha * (sq(a) - sq(b)) / sq(b);
    double A = 1.0 + (usq / 16384.0) * (4096.0 + usq * (-768.0 + usq * (320.0 - 175.0 * usq)));
    double B = (usq / 1024.0) * (256.0 + usq * (-128.0 + usq * (74.0 - 47.0 * usq)));

    double deltaSigma = B * sinSigma * (
        cos2Sigma_m + (B / 4.0) * (
            cosSigma * (-1.0 + 2.0 * sq(cos2Sigma_m)) -
            (B / 6.0) * cos2Sigma_m * (-3.0 + 4.0 * sq(sinSigma)) * (-3.0 + 4.0 * sq(cos2Sigma_m))
        )
    );

    double s = b * A * (sigma - deltaSigma);
    long km = (long)(s / 1000.0);
    long m = (s- km * 1000.0);
    long cm = (long)((s - floor(s)) * 100);
    
    printf("Distance: %ld km, %ld m and %ld cm (iterations: %d)\n", km, m, cm, iter);

    *distance = s;

    return 1;
}


int ListBases(){
        unsigned count = 0;
        Base *bases = load_bases("C:\\vscode\\ballistic\\bin\\bases.txt", &count);
        if (!bases) return -1;

        printf("Loaded %u points:\n", count);
        for (unsigned i = 0; i < count; i++) {
            printf("[%d] [%s] [%.5f', %.5f', %.5f]\n",
                bases[i].info.id,
                bases[i].info.label,
                bases[i].info.position.latitude,
                bases[i].info.position.longitude,
                bases[i].info.position.altitude);
        }

        free(bases);

        return 1;
}

int ListTargets(){
        unsigned count = 0;
        Target *targets = load_targets("C:\\vscode\\ballistic\\bin\\targets.txt", &count);
        if (!targets) return -1;

        printf("Loaded %u points:\n", count);
        for (unsigned i = 0; i < count; i++) {
            printf("[%d] [%s] [%.5f', %.5f', %.5f] [%.2f m2]\n",
                targets[i].info.id,
                targets[i].info.label,
                targets[i].info.position.latitude,
                targets[i].info.position.longitude,
                targets[i].info.position.altitude,
                targets[i].area);
        }

        free(targets);

        return 1;
}
