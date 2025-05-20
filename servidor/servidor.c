#include "transmissor.h"

#define TIMEOUT 5000

#define X_AXIS 0
#define Y_AXIS 1

int storedClientXPos = 0;
int storedClientYPos = 0;

int socket;

int move_character(int axis, int position){
    if(position < 0){
        printf("Invalid movement");
        if(socket != NULL){
            message error = create_message(0, 0, TYPE_ERROR, "");
            message_send(socket, error);    
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

int main(){
    socket = 0;// cria_raw_socket(""); Make connection with client
    message* recieved_message;

    while(1){
        int recieved = message_receive(socket, recieved_message, TIMEOUT);

        if(recieved != -1){
            int computed = message_handler(recieved_message);

            if(computed != -1){
                message ok_msg = create_message(0, 0, TYPE_OKACK, NULL);

                message_send(socket, ok_msg);
            }
        }
    }
}