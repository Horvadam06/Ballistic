#include <stdio.h>

#include "main.h"
#include "debug.h"
#include "commands.h"
#include "calculations.h"
#include "load.h"
#include "error.h"
#include "storage.h"

//=========================================
// Created by.:
// Starting date: 2025.10.17
// Finishing date:
//=========================================

PROCESS Setup()
{
    printf("\n");
    printf("==================================\n");
    printf("Ballistic trajectory calculator\n");
    printf("Created by.: Adam Jozsef Horvath\n");
    printf("==================================\n");
    printf("\n");
    printf("Type [exit] to leave or type [help] for the commands!\n");   

    coordinate pos1 = {
        0.0, 0.0    
        ,0.0
    };

    coordinate pos2 = {
        0.5, 179.7
        ,0.0
    };
//Schönherz 47.4729283,19.0531429     Corvinus gellért 47.4804375,19.0486047
    coordinate start[] = {{47.4729283,19.0531429}, {48.8566, 2.3522}, {51.5074, -0.1278}, {40.7128, -74.0060}, {25.7617, -80.1918}, {35.6895, 139.6917}};
    coordinate end[] = {{47.4804375,19.0486047}, {52.5200, 13.4050}, {55.9533, -3.1883}, {34.0522, -118.2437}, {41.8781, -87.6298}, {37.5665, 126.9780}};

    /*double distance;
    for (int i = 0; i < 6; i++){
        printf("%d ", i);
        CalculateDistance(start[i], end[i], &distance);
        printf("Raw distance: %f meters\n", distance);
    } */

    BaseList bases;
    init_base_list(&bases);

   scan("C:\\vscode\\ballistic\\bin\\bases.txt", handle_base, &bases);
   printf("capacity: %d, items: %d\n", bases.capacity, bases.count);
   for (int i = 0; i<bases.count; i++){
    printf("%d %s\n", bases.items[i].id, bases.items[i].label);

   }

   free_base_list(&bases);

    return S_RUNNING;
}

int main(void)
{
    PROCESS state = Setup();
    while (state == S_RUNNING)
    {
        printf("> ");
        GetCommand(&state);
    }    
    if (state == S_END) printf("Program exited succesfully!\n");
    else if (state == S_TERMINATE) printf("Program terminated!\n");
    return 0;
}