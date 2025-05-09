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
            if(i == currentYPos && j == currentXPos){
                printf("| & ");
            } 
            else {
                printf("|   ");
            }
        }
        printf("|\n");
    }
    for (int j = 0; j < 8; j++) {
        printf("+---");
    }
    printf("+\n");
}

int main(){
    while(1){
        fflush(stdout);
        printGrid();
    
        printf("Para onde gostaria de se mover? Movimentos possiveis: ");
        for(int i = currentYPos -1; i <= currentYPos + 1; i++){
            for(int j = currentXPos-1; j <= currentXPos + 1; j++){
                if((i == currentYPos && j == currentXPos) || (i < 0 || j < 0)){
                    continue;
                }
    
                printf("%d %d, ", i, j);
            }
        }
        printf("\n");
        scanf(" %d %d", &currentYPos, &currentXPos);
    }
    return 0;
}