#include "coordinates.h"
Coord *create_coord(int x, int y){
    Coord *new_coord = (Coord *) malloc(sizeof(Coord));

    if(new_coord == NULL){
        printf("Failed to create coord");
        return NULL;
    }

    new_coord->x = x;
    new_coord->y = y;

    return new_coord;
};

void destroy_coord(Coord *coord){
    free(coord);

    return;
};