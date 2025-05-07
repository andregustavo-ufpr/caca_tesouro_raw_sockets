#include <stdio.h>

int currentXPos = 0;
int currentYPos = 0;

void printGrid() {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            printf("+---");
        }
        printf("+\n");
        for (int j = 0; j < 8; j++) {
            printf("|   ");
        }
        printf("|\n");
    }
    for (int j = 0; j < 8; j++) {
        printf("+---");
    }
    printf("+\n");
}

int main(){
    printGrid();

    printf("Para onde gostaria de se mover?: ");
    printf("Movimentos possiveis: ");
    for(int i = -1; i <= 1; i++){
        for(int j = -1; j <= 1; j++){
            if((i == 0 && j == 0) || (i < 0 || j < 0)){
                continue;
            }

            printf("(%d, %d) ", i, j);
        }
    }

    scanf("%d %d", &currentXPos, &currentYPos);
    return 0;
}