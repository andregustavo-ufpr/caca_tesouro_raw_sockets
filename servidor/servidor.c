#include "../transmissor/transmissor.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dirent.h>

#define MAX_TREASURES 8
// #define USINGLOOPBACK 

typedef struct {
    int x;
    int y;
} Coord;

Coord treasures[MAX_TREASURES];

int storedClientXPos = 0;
int storedClientYPos = 0;

int r_socket;

void find_object(int n, char* out_path){
    DIR* dir = opendir("objects");
    if (!dir) {
        fprintf(stderr, "Falha ao abrir objects\n");
        exit(1);
    }

    struct dirent* entry;
    char prefix[8];
    snprintf(prefix, sizeof(prefix), "%d.", n);

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, prefix, strlen(prefix)) == 0) {
            strcpy(out_path, entry->d_name);
            closedir(dir);
            return;
        }
    }
}

int move_character(int* positionX, int* positionY, int type){
    int newX = *positionX;
    int newY = *positionY;
    switch (type) {
        case TYPE_MOVEDOWN:
            newY++;
            break;
        case TYPE_MOVEUP:
            newY--;
            break;
        case TYPE_MOVERIGHT:
            newX++;
            break;
        case TYPE_MOVELEFT:
            newX--;
            break;
    }

    if (newX < 0 || newX > 7
        || newY < 0 || newY > 7) {
        fprintf(stderr, "Movimento invalido\n");
        return -1;
    } else {
        *positionX = newX;
        *positionY = newY;
    }

    printf("Movimento para %d %d\n", *positionX, *positionY);
    return 0;
}

int message_handler(message* m){
    switch (m->type){
        case TYPE_MOVERIGHT:
        case TYPE_MOVELEFT:
        case TYPE_MOVEUP:
        case TYPE_MOVEDOWN:
            if(move_character(&storedClientXPos, &storedClientYPos, m->type) == -1){
                // In case of an error inside move_character function
                return -2;
            }

            // TODO: check if player is on treasure
            for (int i = 0; i < MAX_TREASURES; ++i) {
                if (treasures[i].x == storedClientXPos && treasures[i].y == storedClientYPos) {
                    return i + 1;
                }
            }
            
            return 0; 
        default:
            fprintf(stderr, "Mensagem de tipo inesperado %d\n", m->type);
            return -1;
    }

    return -1;
}

void choose_random_coordinates(Coord coords[MAX_TREASURES]) {
    int used[9][9] = {0};
    srand(time(NULL));
    int count = 0;
    while (count < MAX_TREASURES) {
        int x = rand() % 8;
        int y = rand() % 8;
        if (!used[x][y]) {
            coords[count].x = x;
            coords[count].y = y;
            used[x][y] = 1;
            count++;
        }
    }
}

int main(int argc, char** argv){
    if (argc < 2) {
        fprintf(stderr, "Especifique uma interface\n");
        exit(1);
    }
    r_socket = cria_raw_socket(argv[1]); // Make connection with client
    message recieved_message;
    memset(&recieved_message, 0, sizeof(message));

    #ifdef USINGLOOPBACK
    message dummy;
    #endif

    choose_random_coordinates(treasures);
    printf("Locais de tesouro:\n");
    for (int i = 0; i < MAX_TREASURES; ++i) {
        printf("%d %d    ", treasures[i].x, treasures[i].y);
    }
    printf("\n");

    while(1){
        #ifdef USINGLOOPBACK
        message_receive(r_socket, &dummy, TIMEOUT);
        #endif
        int recieved = message_receive(r_socket, &recieved_message, TIMEOUT);

        if (compute_checksum(&recieved_message) != recieved_message.checksum) {
            printf("checksum failed\n");
            continue;
        }

        if(recieved != -1){
            printf("received type %d\n", recieved_message.type);
            int computed = message_handler(&recieved_message);

            
            if (computed > 0) {
                char object_name[64];
                printf("Tesouro %d encontrado\n", computed);
                find_object(computed, object_name);
                printf("Tesouro: %s\n", object_name);
            }
            else if (computed == 0) {
                message ok_msg = create_message(0, 0, TYPE_OKACK, NULL);

                message_send(r_socket, ok_msg);
                #ifdef USINGLOOPBACK
                message_receive(r_socket, &dummy, TIMEOUT);
                #endif
            } else if (computed == -2) {
                message err_msg = create_message(0, 0, TYPE_ERROR, NULL);

                message_send(r_socket, err_msg);
                #ifdef USINGLOOPBACK
                message_receive(r_socket, &dummy, TIMEOUT);
                #endif
                exit(1);
            }
        }
    }
}
