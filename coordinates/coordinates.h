#include <stdio.h>

typedef struct {
    int x;
    int y;
} Coord;

Coord *create_coord(int x, int y);

void destroy_coord(Coord *coord);