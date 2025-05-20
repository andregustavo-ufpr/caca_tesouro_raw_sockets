#include "transmissor.h"

#define TIMEOUT 5000

int storedClientXPos = 0;
int storedClientYPos = 0;

int message_handler(message* m){
    switch (m->type){
        case TYPE_MOVERIGHT:
            storedClientXPos = (int) m->data;
            return 0;
        case TYPE_MOVELEFT:
            storedClientXPos = - (int) m->data;
            return 0;
        case TYPE_MOVEUP:
            storedClientYPos = (int) m->data;
            return 0;
        case TYPE_MOVEDOWN:
            storedClientYPos = - (int) m->data;
            return 0;
        default:
            return -1;
    }
}

int main(){
    int socket = 0;// cria_raw_socket(""); Make connection with client
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