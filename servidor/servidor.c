#include "transmissor.h"
#include <time.h>

#define MAX_TREASURES 9

#define X_AXIS 0
#define Y_AXIS 1

typedef struct {
    int x;
    int y;
} Coord;

Coord treasures[MAX_TREASURES];

int storedClientXPos = 0;
int storedClientYPos = 0;

int r_socket;

int move_character(int axis, int position){
    if(position < 0){
        printf("Invalid movement");
        if(r_socket != 0){
            unsigned char *empty_data = NULL;
            message error = create_message(0, 0, TYPE_ERROR, empty_data);
            message_send(r_socket, error);    
        }

        return -1;
    }

    if(axis == X_AXIS){
        storedClientXPos += position;
    }
    else{
        storedClientYPos += position;
    }

    return 0;
}

int message_handler(message* m){
    switch (m->type){
        case TYPE_MOVERIGHT:
        case TYPE_MOVELEFT:
            if(move_character(X_AXIS, (int) m->data) == -1){
                // In case of an error inside move_character function
                break;
            }
            
            return 0;
        case TYPE_MOVEUP:
        case TYPE_MOVEDOWN:
            if(move_character(Y_AXIS, (int) m->data) == -1){\
                // In case of an error inside move_character function
                break;
            }

            return 0;
        default:
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

int main(){
    r_socket = cria_raw_r_socket(""); // Make connection with client
    message* recieved_message = NULL;

    choose_random_coordinates(treasures);

    while(1){
        int recieved = message_receive(r_socket, recieved_message, TIMEOUT);

        if(recieved != -1){
            int computed = message_handler(recieved_message);

            if(computed != -1){
                unsigned char *empty_data = NULL;
                message ok_msg = create_message(0, 0, TYPE_OKACK, empty_data);

                message_send(r_socket, ok_msg);
            }
        }
    }
}