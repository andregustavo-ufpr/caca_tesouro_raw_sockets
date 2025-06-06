#include "../transmissor/transmissor.h"
#include <string.h>
#include <time.h>

#define MAX_TREASURES 9
#define USINGLOOPBACK 

typedef struct {
    int x;
    int y;
} Coord;

Coord treasures[MAX_TREASURES];

int storedClientXPos = 0;
int storedClientYPos = 0;

int r_socket;

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
                break;
            }

            // TODO: check if player is on treasure
            
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
        int x = rand() % MAX_TREASURES;
        int y = rand() % MAX_TREASURES;
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

            if(computed != -1){
                message ok_msg = create_message(0, 0, TYPE_OKACK, NULL);

                message_send(r_socket, ok_msg);
                #ifdef USINGLOOPBACK
                message_receive(r_socket, &dummy, TIMEOUT);
                #endif
            } else {
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
